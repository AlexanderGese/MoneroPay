# Monero Payment API

A secure, efficient, and production-ready REST API for managing Monero cryptocurrency payments using a single master wallet.

![Monero](https://img.shields.io/badge/cryptocurrency-Monero-orange)
![License](https://img.shields.io/badge/license-MIT-blue)
![C++](https://img.shields.io/badge/language-C%2B%2B-blue)
![Crow](https://img.shields.io/badge/framework-Crow-black)

## Overview

This API provides a simple interface for e-commerce platforms, services, and applications to accept Monero payments without having to handle the complexities of cryptocurrency wallet management. It uses the Monero Wallet RPC to interact with the Monero network and provides REST endpoints for address generation and payment verification.

### Features

- **Create Payment Addresses**: Generate unique Monero addresses for each customer/transaction
- **Verify Payments**: Check if payments have been received with configurable confirmation requirements
- **Single Master Wallet**: All funds go to a single master wallet for easy management
- **Thread-safe**: Designed for concurrent operations
- **Secure**: API key authentication, input validation, and proper error handling
- **Production-ready**: Configurable, robust error handling, and health monitoring

## Prerequisites

- C++11 compatible compiler (GCC, Clang, MSVC)
- CMake 3.10+
- Monero CLI wallet (v0.17.0.0+)
- libcurl
- nlohmann/json
- Crow (C++ web framework)

## Installation

### Clone the Repository

```bash
git clone https://github.com/AlexanderGese/MoneroPay.git
cd MoneroPay
```

### Install Dependencies

#### Ubuntu/Debian

```bash
# Install system dependencies
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev

# Install Crow (header-only library)
git clone https://github.com/CrowCpp/Crow.git
cd Crow
mkdir build && cd build
cmake ..
make && sudo make install
cd ../../
```

#### macOS (using Homebrew)

```bash
brew update
brew install cmake curl nlohmann-json
brew install wget

# Install Crow
wget https://github.com/CrowCpp/Crow/releases/download/v1.0/crow_all.h
sudo mkdir -p /usr/local/include/crow
sudo mv crow_all.h /usr/local/include/crow/
```

#### Windows (using vcpkg)

```powershell
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install
./vcpkg install curl nlohmann-json crow
```

### Build the API

```bash
mkdir build && cd build
cmake ..
make
```

## Monero Wallet Setup

### Create a Wallet (if you don't have one)

```bash
monero-wallet-cli --generate-new-wallet=my_wallet
```

### Start the Monero Wallet RPC

```bash
monero-wallet-rpc --wallet-file=my_wallet --rpc-bind-port=18082 --rpc-login=monero:your_secure_password_here --daemon-address=node.monero.org:18081 --confirm-external-bind
```

## Configuration

Create a `config.json` file in the same directory as the executable with the following structure:

```json
{
  "monero_rpc": {
    "url": "http://127.0.0.1:18082/json_rpc",
    "username": "monero",
    "password": "your_secure_password_here"
  },
  "api": {
    "port": 8080,
    "key": "your_secure_api_key_here"
  },
  "monero": {
    "min_confirmations": 10
  }
}
```

| Configuration Option | Description |
|----------------------|-------------|
| `monero_rpc.url` | URL for the Monero Wallet RPC server |
| `monero_rpc.username` | RPC username (if authentication is enabled) |
| `monero_rpc.password` | RPC password (if authentication is enabled) |
| `api.port` | Port the API will listen on |
| `api.key` | Secret key for API authentication |
| `monero.min_confirmations` | Minimum confirmations required to consider a payment verified |

## Usage

### Start the API

```bash
./monero_payment_api
```

### API Endpoints

#### Health Check

```
GET /health
```

Example:
```bash
curl "http://localhost:8080/health"
```

Response:
```json
{
  "status": "ok",
  "timestamp": 1648236024123
}
```

#### Generate a New Payment Address

```
POST /create_address?api_key=your_secure_api_key_here
```

Request Body:
```json
{
  "label": "customer123",
  "amount": 0.5
}
```

Example:
```bash
curl -X POST "http://localhost:8080/create_address?api_key=your_secure_api_key_here" \
  -H "Content-Type: application/json" \
  -d '{"label": "customer123", "amount": 0.5}'
```

Response:
```json
{
  "address": "4BrL51JCc9NGQ71kWhnYoDRffsDZy7m1HUU7MRU4nUMXAHNFBEJhkTZV9HdaL4gfuNBxLPc3BeMkLGaPbF5vWtANQn5neoXiXECnZ",
  "amount": 0.5
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | string | Optional label for the address (e.g., customer ID, order ID) |
| `amount` | float | Amount in XMR that should be paid to this address |

#### Verify a Payment

```
POST /verify_payment?api_key=your_secure_api_key_here
```

Request Body:
```json
{
  "address": "4BrL51JCc9NGQ71kWhnYoDRffsDZy7m1HUU7MRU4nUMXAHNFBEJhkTZV9HdaL4gfuNBxLPc3BeMkLGaPbF5vWtANQn5neoXiXECnZ"
}
```

Example:
```bash
curl -X POST "http://localhost:8080/verify_payment?api_key=your_secure_api_key_here" \
  -H "Content-Type: application/json" \
  -d '{"address": "4BrL51JCc9NGQ71kWhnYoDRffsDZy7m1HUU7MRU4nUMXAHNFBEJhkTZV9HdaL4gfuNBxLPc3BeMkLGaPbF5vWtANQn5neoXiXECnZ"}'
```

Response:
```json
{
  "verified": true,
  "address": "4BrL51JCc9NGQ71kWhnYoDRffsDZy7m1HUU7MRU4nUMXAHNFBEJhkTZV9HdaL4gfuNBxLPc3BeMkLGaPbF5vWtANQn5neoXiXECnZ",
  "expected_amount": 0.5
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `address` | string | The Monero address to verify payment for |

## Integration Guide

### Typical Flow

1. **Generate a payment address**:
   - When a customer wants to pay, call the `/create_address` endpoint with the required amount
   - Store the returned address and amount in your database, associated with the order

2. **Display payment instructions**:
   - Show the generated Monero address to the customer
   - Specify the exact amount they need to send

3. **Check for payment**:
   - Periodically call the `/verify_payment` endpoint with the address
   - When verified (returns `"verified": true`), update your order status

4. **Complete the transaction**:
   - Once payment is verified, deliver goods or services to the customer
   - The API automatically removes verified payments from its tracking system

### Example Integration (JavaScript/Node.js)

```javascript
const axios = require('axios');

const API_URL = 'http://localhost:8080';
const API_KEY = 'your_secure_api_key_here';

async function createPaymentAddress(orderId, amount) {
  try {
    const response = await axios.post(`${API_URL}/create_address?api_key=${API_KEY}`, {
      label: `order-${orderId}`,
      amount: amount
    });
    
    return response.data;
  } catch (error) {
    console.error('Failed to create payment address:', error.response?.data || error.message);
    throw error;
  }
}

async function verifyPayment(address) {
  try {
    const response = await axios.post(`${API_URL}/verify_payment?api_key=${API_KEY}`, {
      address: address
    });
    
    return response.data.verified;
  } catch (error) {
    console.error('Failed to verify payment:', error.response?.data || error.message);
    throw error;
  }
}

// Usage example
async function processOrder(orderId, amount) {
  // 1. Create a payment address for the order
  const payment = await createPaymentAddress(orderId, amount);
  console.log(`Payment address for order ${orderId}: ${payment.address}`);
  
  // 2. Store the address in your database
  // saveAddressToDatabase(orderId, payment.address, payment.amount);
  
  // 3. Periodically check for payment (e.g., every 5 minutes)
  const paymentCheckInterval = setInterval(async () => {
    const isPaid = await verifyPayment(payment.address);
    
    if (isPaid) {
      console.log(`Payment received for order ${orderId}!`);
      clearInterval(paymentCheckInterval);
      
      // 4. Update order status and fulfill order
      // updateOrderStatus(orderId, 'paid');
      // fulfillOrder(orderId);
    }
  }, 5 * 60 * 1000);
}
```

## Security Considerations

- **API Key Protection**: Keep your API key secure and rotate it regularly
- **HTTPS**: Always use HTTPS in production environments with a valid SSL certificate
- **IP Restrictions**: Consider implementing IP whitelisting for additional security
- **Error Logging**: Monitor error logs for suspicious activity
- **Wallet Security**: Secure your master wallet with a strong password and consider using a hardware wallet for large amounts
- **Regular Backups**: Backup your wallet seed phrase and regularly backup the wallet file
- **Updates**: Keep your Monero software and this API updated with the latest security patches

## Monitoring and Maintenance

- **Health Endpoint**: Use the `/health` endpoint to monitor the API's status
- **Regular Balance Checks**: Periodically check your wallet balance to ensure all funds are accounted for
- **Log Rotation**: Implement log rotation to prevent disk space issues
- **Database Integration**: Consider extending the API to store payment information in a database for better persistence
- **Automated Testing**: Implement automated tests to ensure the API functions correctly after updates

## Error Handling

The API includes comprehensive error handling with appropriate HTTP status codes:

| Status Code | Description |
|-------------|-------------|
| 200 | Success |
| 400 | Bad request (invalid parameters) |
| 401 | Unauthorized (invalid or missing API key) |
| 404 | Resource not found |
| 500 | Server error (Monero wallet issues, etc.) |

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This software is provided "as is", without warranty of any kind. Use it at your own risk. The authors or copyright holders shall not be liable for any claim, damages, or other liability arising from the use of the software.

## Acknowledgements

- The Monero Project (https://www.getmonero.org/)
- CrowCpp (https://crowcpp.org)
- JSON for Modern C++ (https://github.com/nlohmann/json)
