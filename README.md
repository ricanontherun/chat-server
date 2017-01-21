# Chat Server

A simple chat server who's job is to route messages from a sender to all connected clients. This is an exercise in networked applications.

## Dependencies

* C++11
* ZeroMQ

Ports used (TODO: Make configurable?)

* 5000: Message receive
* 5001: New client connections
* 5002: Message broadccast

## Client-Server Message Flow

Once the server is running, the following series of communication takes place from client to server.

### Send a connection request to the server:

```json
{
	username: YOUR_USERNAME
}
```

If successful, the client will then receive:
```json
{
	success: true,
    token: YOUR_TOKEN
}
```

If an error occured, the reply will be:
```json
{
	success:false,
    error: "Error explaination"
}
```

## Send a message

Now that the client has their token, they can send messages to the server, which will be routed to each connected client.

Messges should be send in the following form:
```json
{
	token: YOUR_TOKEN,
    message: "Yo!"
}
```

Clients should have a background thread connected to port 5002 to receive messages from other clients which will be in the form:


```json
{
	username: SENDER_USERNAME,
    message: "Hey!"
}
```
