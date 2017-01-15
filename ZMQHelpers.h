#ifndef ZMQ_HELPERS
#define ZMQ_HELPERS

/**
 * Extract a null terminated string from a zmq message.
 *
 * @param message
 * @param buffer
 */
void zmq_extract_message(const zmq::message_t &message, std::string &buffer) {
  std::size_t size = message.size();

  char *buf = (char *) malloc(size);

  memcpy((void *) buf, message.data(), size);

  buffer.assign(buf, size);

  free(buf);
}

void zmq_pack_message(zmq::message_t & message, std::string data)
{
  std::size_t data_length = data.length();

  message.rebuild(data_length);

  memcpy(message.data(), data.c_str(), data_length);
}

#endif