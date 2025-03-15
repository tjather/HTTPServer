# HTTPServer

## Overview  
This project implements a **multi-threaded TCP web server** in C that can handle multiple simultaneous HTTP requests. The server responds to `GET` requests, serving static files from a predefined **document root (`./www`)** while ensuring correct HTTP versioning and error handling.  

## Features  
- **Multi-threaded architecture** for handling multiple client connections concurrently.  
- **HTTP/1.0 and HTTP/1.1 support**, responding with the correct protocol version.  
- **Default document handling**, serving `index.html` if a directory is requested.  
- **Comprehensive error handling** with appropriate HTTP status codes:  
  - `200 OK` – Successful request.  
  - `400 Bad Request` – Malformed request.  
  - `403 Forbidden` – Insufficient file permissions.  
  - `404 Not Found` – Requested file does not exist.  
  - `405 Method Not Allowed` – Only `GET` requests are supported.  
  - `505 HTTP Version Not Supported` – Only HTTP/1.0 and HTTP/1.1 are supported.  

## Components  
### **Server (`tcp_server.c`)**  
- Listens for incoming connections on a specified port.  
- Uses **multi-threading (`pthread`)** to handle multiple clients simultaneously.  
- Parses **HTTP GET requests**, retrieves requested files from the document root (`./www`), and sends appropriate responses.  
- Implements a **timeout mechanism** for keep-alive connections.  

### **Client (`tcp_client.c`)**  
- Connects to the server and sends HTTP requests.  
- Receives and displays responses from the server.  
