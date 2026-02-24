# Chat Over TCP

A simple **multi-client chatroom** built in **C** using raw **TCP sockets** and **POSIX threads**.
It allows multiple clients to connect to a server, exchange messages in real time, and see when participants join or leave.

---

## ✨ Features

* Real-time public and **private messaging**.
* Usernames are sent once on connection.
* **List all active users**.
* Colored terminal output for join/leave notifications and messages.
* Graceful handling of client disconnections.
* Lightweight: only uses C standard libraries + POSIX sockets and threads.

---

## 📂 Project Structure

```
.
├── Client
│   └── client.c
├── Server
│   ├── connection.c
│   ├── connection.h
│   ├── main.c
│   ├── message.c
│   └── message.h
├── makefile
├── README.md
└── utils
    ├── colors.h
    ├── socketutils.c
    └── socketutils.h
```

---

## ⚙️ How It Works

1. **Server**

* Listens for incoming connections.
* Assigns each client a slot in `acceptedSockets[]`.
* Spawns a thread per client to handle incoming messages.
* Broadcasts all messages to all clients (except the sender).
* Handles private messages by looking for a `PVTMSG` prefix and forwarding the message to the intended recipient.

2. **Client**

* Connects to the server and sends a username.
* Spawns a thread to continuously listen for incoming messages.
* Main thread handles user input (`getline`), sending messages until `exit`.
* Supports commands like `/users` to see other users and `/chat <username>` to start a private chat.

---

## 🚀 Getting Started

### 1. Compile

```bash
make server
make client
```

### 2. Run the Server

```bash
./server
```

### 3. Run Clients (in separate terminals)

```bash
./client
```

Enter your name when prompted, then start chatting.
Type `/help` to see the available commands.

---

## 🛠️ Future Improvements

* Support for file transfers.
* Authentication (password-based login).
* Replace thread-per-client model with `select()`/`poll()` for scalability.
* Docker container for easy setup.
