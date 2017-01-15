#include <iostream>
#include <sstream>
#include <functional>
#include <string.h>
#include <map>

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

    // Someone has sent a message.
    // We receive it and publish it to the channel.
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

      zmq_extract_message(incoming_message, connection_username);
      std::stringstream ss(connection_username);

      ss >> key >> connection_username;

      if ( strcmp(key.c_str(), CLIENT_CONNECT_KEY) != 0 ) {
        continue;
      } else {
        std::stringstream hash_stream;
        std::size_t hash = uname_hash(connection_username);
        hash_stream << hash;

        zmq_pack_message(outgoing_message, "CLIENT_TOKEN " + hash_stream.str());
        client_connection.send(outgoing_message);
      }
    }
  }

  return EXIT_SUCCESS;
}