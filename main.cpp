#include <iostream>
#include <sstream>
#include <functional>
#include <string.h>
#include <map>

#include "json.hpp"
#include "zmq.hpp"
#include "ZMQHelpers.h"

#define CLIENT_CONNECT_KEY "CLIENT_CONNECT"

int main()
{
  std::map<std::size_t, std::string> connection_map;
  std::hash<std::string> uname_hash;

  zmq::context_t context(1);

  // Socket which listens for messages.
  zmq::socket_t message_receiver(context, ZMQ_PULL);

  // Socket which listens for new client connections.
  zmq::socket_t client_connection(context, ZMQ_REP);

  zmq::socket_t publish(context, ZMQ_PUB);

  // Bind the server to 5000, which clients will send their messages to.
  try {
    message_receiver.bind("tcp://*:5000");
    client_connection.bind("tcp://*:5001");
    publish.bind("tcp://*:5002");
  } catch ( zmq::error_t & error ) {
    std::cerr << "Failed to bind, " << error.what() << "\n";
    return EXIT_FAILURE;
  }

  // Initialize poll set
  zmq::pollitem_t items[] = {
      {message_receiver, 0, ZMQ_POLLIN, 0},
      {client_connection, 0, ZMQ_POLLIN, 0}
  };

  zmq::message_t incoming_message;
  zmq::message_t outgoing_message;
  std::string incoming_message_string;

  while (true) {
    zmq::poll(&items[0], 2, -1);

    if ( items[0].revents & ZMQ_POLLIN ) {
      message_receiver.recv(&incoming_message);
      zmq_extract_message(incoming_message, incoming_message_string);

      zmq_pack_message(outgoing_message, "MESSAGE " + incoming_message_string);

      publish.send(outgoing_message);
    }

    // NEW CONNECTION
    if ( items[1].revents & ZMQ_POLLIN ) {
      std::string key;
      std::string connection_username;
      client_connection.recv(&incoming_message); // We HAVE to receive the message.

      nlohmann::json response;
      response["success"] = true;

      zmq_extract_message(incoming_message, connection_username);

      try {
        auto json = nlohmann::json::parse(connection_username);

        // Check and grab the username field.
        if ( json.count("username") == 0 ) {
          response["success"] = false;
          response["error"] = "Missing 'username' field";
        } else {
          std::string username = json["username"];
          std::size_t hash = uname_hash(username);

          response["token"] = hash;
          connection_map[hash] = username;
        }

        zmq_pack_message(outgoing_message, response.dump());
        client_connection.send(outgoing_message);

      } catch ( std::invalid_argument & e) {
        response["success"] = false;
        response["error"] = "Failed to parse JSON";

        zmq_pack_message(outgoing_message, response.dump());

        client_connection.send(outgoing_message);
      }
    }
  }

  return EXIT_SUCCESS;
}