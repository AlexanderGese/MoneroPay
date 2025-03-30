#include <crow.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;

class MoneroWalletRPC {
private:
    std::string rpc_url;
    std::string rpc_username;
    std::string rpc_password;
    std::mutex wallet_mutex;
    
    // Callback function for CURL to write response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        try {
            s->append((char*)contents, newLength);
            return newLength;
        } catch(std::bad_alloc& e) {
            return 0;
        }
    }

    // Helper to make RPC calls to Monero wallet
    json make_rpc_call(const std::string& method, const json& params) {
        std::lock_guard<std::mutex> lock(wallet_mutex);
        
        CURL* curl = curl_easy_init();
        std::string response_string;
        
        if (curl) {
            json request;
            request["jsonrpc"] = "2.0";
            request["id"] = "0";
            request["method"] = method;
            request["params"] = params;
            
            std::string request_string = request.dump();
            
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            
            curl_easy_setopt(curl, CURLOPT_URL, rpc_url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_string.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            
            // Set authentication if provided
            if (!rpc_username.empty() && !rpc_password.empty()) {
                curl_easy_setopt(curl, CURLOPT_USERNAME, rpc_username.c_str());
                curl_easy_setopt(curl, CURLOPT_PASSWORD, rpc_password.c_str());
            }
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res != CURLE_OK) {
                std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw std::runtime_error("Failed to connect to Monero wallet RPC");
            }
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        } else {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        try {
            json response = json::parse(response_string);
            if (response.contains("error")) {
                throw std::runtime_error("RPC error: " + response["error"]["message"].get<std::string>());
            }
            return response["result"];
        } catch (json::parse_error& e) {
            throw std::runtime_error("Failed to parse RPC response: " + response_string);
        }
    }
    
public:
    MoneroWalletRPC(const std::string& url, const std::string& username = "", const std::string& password = "")
        : rpc_url(url), rpc_username(username), rpc_password(password) {
        // Initialize CURL globally
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~MoneroWalletRPC() {
        // Clean up CURL
        curl_global_cleanup();
    }
    
    // Generate a new address with optional payment ID
    std::string create_address(const std::string& label = "", const std::string& payment_id = "") {
        json params;
        params["account_index"] = 0;
        params["label"] = label;
        
        if (!payment_id.empty()) {
            params["payment_id"] = payment_id;
        }
        
        try {
            json result = make_rpc_call("create_address", params);
            return result["address"];
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to create address: " + std::string(e.what()));
        }
    }
    
    // Check if a payment has been received
    bool verify_payment(const std::string& address, const double amount, const int min_confirmations = 10) {
        try {
            // Get all transfers to the address
            json params;
            params["in"] = true;
            params["out"] = false;
            
            json transfers = make_rpc_call("get_transfers", params);
            
            if (!transfers.contains("in")) {
                return false;
            }
            
            double received_amount = 0;
            
            for (const auto& transfer : transfers["in"]) {
                if (transfer["address"] == address) {
                    if (transfer["confirmations"] >= min_confirmations) {
                        // Convert atomic units to XMR
                        double transfer_amount = static_cast<double>(transfer["amount"].get<uint64_t>()) / 1e12;
                        received_amount += transfer_amount;
                    }
                }
            }
            
            return received_amount >= amount;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to verify payment: " + std::string(e.what()));
        }
    }
    
    // Get the current balance
    double get_balance() {
        try {
            json params;
            params["account_index"] = 0;
            
            json result = make_rpc_call("get_balance", params);
            
            // Convert atomic units to XMR
            return static_cast<double>(result["balance"].get<uint64_t>()) / 1e12;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get balance: " + std::string(e.what()));
        }
    }
};

// Store of pending payments
class PaymentStore {
private:
    std::unordered_map<std::string, double> pending_payments;
    std::mutex store_mutex;
    
public:
    void add_payment(const std::string& address, double amount) {
        std::lock_guard<std::mutex> lock(store_mutex);
        pending_payments[address] = amount;
    }
    
    bool get_payment_details(const std::string& address, double& amount) {
        std::lock_guard<std::mutex> lock(store_mutex);
        if (pending_payments.find(address) != pending_payments.end()) {
            amount = pending_payments[address];
            return true;
        }
        return false;
    }
    
    void remove_payment(const std::string& address) {
        std::lock_guard<std::mutex> lock(store_mutex);
        pending_payments.erase(address);
    }
};

// Configuration class
class Config {
private:
    json config_data;
    
public:
    Config(const std::string& config_file = "config.json") {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + config_file);
        }
        file >> config_data;
    }
    
    std::string get_rpc_url() const {
        return config_data["monero_rpc"]["url"];
    }
    
    std::string get_rpc_username() const {
        return config_data["monero_rpc"]["username"];
    }
    
    std::string get_rpc_password() const {
        return config_data["monero_rpc"]["password"];
    }
    
    std::string get_api_key() const {
        return config_data["api"]["key"];
    }
    
    int get_port() const {
        return config_data["api"]["port"];
    }
    
    int get_min_confirmations() const {
        return config_data["monero"]["min_confirmations"];
    }
};

// API Authentication middleware
struct AuthMiddleware {
    std::string api_key;
    
    AuthMiddleware(std::string key) : api_key(std::move(key)) {}
    
    struct context {};
    
    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        if (req.url_params.get("api_key") == nullptr || req.url_params.get("api_key") != api_key) {
            res.code = 401;
            res.end();
            res.skip_all();
        }
    }
    
    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nothing to do after handling
    }
};

int main() {
    try {
        // Load configuration
        Config config;
        
        // Initialize Monero wallet RPC
        MoneroWalletRPC wallet(
            config.get_rpc_url(),
            config.get_rpc_username(),
            config.get_rpc_password()
        );
        
        // Initialize payment store
        PaymentStore payment_store;
        
        // Set up the Crow app with auth middleware
        crow::App<AuthMiddleware> app(AuthMiddleware(config.get_api_key()));
        
        // Enable CORS
        auto& cors = app.get_middleware<crow::CORSHandler>();
        cors
            .global()
            .headers("Content-Type")
            .methods("POST", "GET", "OPTIONS");
        
        // Logging
        app.loglevel(crow::LogLevel::INFO);
        
        // Health check endpoint (no auth required)
        CROW_ROUTE(app, "/health")
        .methods("GET"_method)
        ([&]() {
            json response;
            response["status"] = "ok";
            response["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            
            return crow::response(200, response.dump());
        });
        
        // Generate new Monero address endpoint
        CROW_ROUTE(app, "/create_address")
        .methods("POST"_method)
        ([&](const crow::request& req) {
            try {
                auto body = json::parse(req.body);
                std::string label = body.value("label", "");
                double amount = body.value("amount", 0.0);
                
                if (amount <= 0) {
                    json error;
                    error["error"] = "Invalid amount. Must be greater than 0.";
                    return crow::response(400, error.dump());
                }
                
                // Generate a new address
                std::string address = wallet.create_address(label);
                
                // Store the expected payment
                payment_store.add_payment(address, amount);
                
                json response;
                response["address"] = address;
                response["amount"] = amount;
                
                return crow::response(200, response.dump());
            } catch (const std::exception& e) {
                json error;
                error["error"] = e.what();
                return crow::response(500, error.dump());
            }
        });
        
        // Verify payment endpoint
        CROW_ROUTE(app, "/verify_payment")
        .methods("POST"_method)
        ([&](const crow::request& req) {
            try {
                auto body = json::parse(req.body);
                std::string address = body["address"];
                
                // Get the expected payment amount
                double expected_amount;
                if (!payment_store.get_payment_details(address, expected_amount)) {
                    json error;
                    error["error"] = "Address not found in pending payments.";
                    return crow::response(404, error.dump());
                }
                
                // Verify the payment
                bool is_paid = wallet.verify_payment(
                    address,
                    expected_amount,
                    config.get_min_confirmations()
                );
                
                json response;
                response["verified"] = is_paid;
                response["address"] = address;
                response["expected_amount"] = expected_amount;
                
                // Remove from pending payments if verified
                if (is_paid) {
                    payment_store.remove_payment(address);
                }
                
                return crow::response(200, response.dump());
            } catch (const std::exception& e) {
                json error;
                error["error"] = e.what();
                return crow::response(500, error.dump());
            }
        });
        
        // Start the server
        app.port(config.get_port())
            .multithreaded()
            .run();
            
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
