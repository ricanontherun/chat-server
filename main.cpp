#include <iostream>
#include <sstream>
#include <functional>
#include <map>

#include "json.hpp"
#include "zmq.hpp"
#include "ZMQHelpers.h"

int main() {
  // We're using an ordered map for O(log(n)) user lookups.
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
  } catch (zmq::error_t &error) {
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

    // Incoming client message.
    // 1) Extract the token, verify the token exists to a client
    // 2) Publish
    if (items[0].revents & ZMQ_POLLIN) {
      message_receiver.recv(&incoming_message);

      nlohmann::json request;
      nlohmann::json response;

      response["success"] = true;

      if (!zmq_extract_json(incoming_message, request)) {
        response["success"] = false;
        response["error"] = "Invalid JSON";
      } else {
        nlohmann::json::iterator token_it = request.find("token");
        nlohmann::json::iterator message_it = request.find("message");

        // We want to make sure the token and message fields are present in the
        // json string.
        if (token_it != request.end() && message_it != request.end() ) {
          std::size_t token = *token_it;

          // Look up the user in the connection map.
          auto user_it = connection_map.find(token);
          if ( user_it != connection_map.end() ) {
            response["username"] = user_it->second;
            response["message"] = *message_it;
          } else {
            response["success"] = false;
            response["error"] = "Invalid token, unconnected user.";
          }
        } else {
          response["success"] = false;
          response["error"] = "Invalid request.";
        }
      }

      zmq_pack_message(outgoing_message, "MESSAGE " + response.dump());
      publish.send(outgoing_message);
    }

    // NEW CONNECTION
    if (items[1].revents & ZMQ_POLLIN) {
      std::string key;
      std::string connection_username;
      client_connection.recv(&incoming_message); // We HAVE to receive the message.

      nlohmann::json json;
      nlohmann::json response;
      response["success"] = true;

      if (!zmq_extract_json(incoming_message, json)) {
        // Failed to parse JSON, send back error.
        response["success"] = false;
        response["error"] = "Invalid json";

        zmq_pack_message(outgoing_message, response.dump());
        client_connection.send(outgoing_message);
      }

      // Check and grab the username field.
      nlohmann::json::iterator match = json.find("username");

      if (match != json.end()) {
        std::string username = *match;
        std::size_t hash = uname_hash(username);

        response["token"] = hash;
        connection_map[hash] = username;
      } else {
        response["success"] = false;
        response["error"] = "Missing 'username' field";
      }

      // Return the client token.
      zmq_pack_message(outgoing_message, response.dump());
      client_connection.send(outgoing_message);
    }
  }

  return EXIT_SUCCESS;
}