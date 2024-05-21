#include "./DNSServer.h"
#include "coap-simple.h"
#include <cppQueue.h>

#define DEBUG_OUTPUT Serial

#define SIZECLASS 2
#define SIZETYPE 2
#define DATALENGTH 4


void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Coap Response got]");
  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  Serial.println(p);
}


DNSServer::DNSServer() {
  WiFiUDP udp;

  _ttl = htonl(60);
  _coap = new Coap(udp);

  _coap->response(callback_response);
  _coap->start();
}


bool DNSServer::start(const uint16_t port, const String &upstream_doh) {
  _upstream_doh = upstream_doh;
  if (_udp.listen(port)) {
    _udp.onPacket(
            [&](AsyncUDPPacket &packet) {
              this->processRequest(packet);
            }
    );
    return true;
  }
  return false;
}

void DNSServer::checkToResponse(){
  _coap->loop();
}


void DNSServer::setTTL(const uint32_t &ttl) {
  _ttl = htonl(ttl);
}

void DNSServer::stop() {
  _udp.close();
}


void DNSServer::processRequest(AsyncUDPPacket &packet) {
  if (packet.length() >= sizeof(DNSHeader)) {
    Serial.println("got request");
    unsigned char *_buffer = packet.data();
    DNSHeader *_dnsHeader = (DNSHeader *) _buffer;
    size_t qnameLength = 0;

    String domainNameWithoutWwwPrefix = (_buffer == nullptr ? "" : getDomainNameWithoutWwwPrefix(
            _buffer + sizeof(DNSHeader), qnameLength));

    if (_dnsHeader->QR == DNS_QR_QUERY &&
        _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
        requestIncludesOnlyOneAQuestion(packet, qnameLength) &&
        domainNameWithoutWwwPrefix.length() > 1
            ) {

      String ipStr = askServerForIp(domainNameWithoutWwwPrefix);
      ipStr = "1.1.1.1";


      if (ipStr.length() > 4) {
        IPAddress resolvedIP;
        resolvedIP.fromString(ipStr);

        replyWithIP(packet, resolvedIP, qnameLength);

      } else {
        replyWithCustomCode(packet, qnameLength, DNSReplyCode::NonExistentDomain);
      }

    } else if (_dnsHeader->QR == DNS_QR_QUERY) {
      replyWithCustomCode(packet, qnameLength, DNSReplyCode::Refused);
    }
  }
}


void DNSServer::replyWithIP(AsyncUDPPacket &packet, IPAddress &resolvedIP, size_t &_qnameLength) {
  unsigned char paresedResolvedIP[4];
  paresedResolvedIP[0] = resolvedIP[0];
  paresedResolvedIP[1] = resolvedIP[1];
  paresedResolvedIP[2] = resolvedIP[2];
  paresedResolvedIP[3] = resolvedIP[3];


  // DNS Header + qname + Type +  Class + qnamePointer  + TYPE + CLASS + TTL + Datalength ) IP
  // sizeof(DNSHeader) + _qnameLength  + 2*SIZECLASS +2*SIZETYPE + sizeof(_ttl) + DATLENTHG + sizeof(_resolvedIP)
  AsyncUDPMessage msg(sizeof(DNSHeader) + _qnameLength + 2 * SIZECLASS + 2 * SIZETYPE + sizeof(_ttl) + DATALENGTH +
                      sizeof(paresedResolvedIP));

  msg.write(packet.data(), sizeof(DNSHeader) + _qnameLength + 4); // Question Section included.
  DNSHeader *_dnsHeader = (DNSHeader *) msg.data();

  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->ANCount = htons(1);
  _dnsHeader->QDCount = _dnsHeader->QDCount;
  _dnsHeader->ARCount = 0;

  msg.write((uint8_t) 192); //  answer name is a pointer
  msg.write((uint8_t) 12);  // pointer to offset at 0x00c

  msg.write((uint8_t) 0);   // 0x0001  answer is type A query (host address)
  msg.write((uint8_t) 1);

  msg.write((uint8_t) 0);   //0x0001 answer is class IN (internet address)
  msg.write((uint8_t) 1);

  msg.write((uint8_t * ) & _ttl, sizeof(_ttl));

  // Length of RData is 4 bytes (because, in this case, RData is IPv4)
  msg.write((uint8_t) 0);
  msg.write((uint8_t) 4);
  msg.write(paresedResolvedIP, sizeof(paresedResolvedIP));

  packet.send(msg);

}

void DNSServer::replyWithCustomCode(AsyncUDPPacket &packet, size_t &_qnameLength, DNSReplyCode replyCode) {
  AsyncUDPMessage msg(sizeof(DNSHeader));

  msg.write(packet.data(), sizeof(DNSHeader)); // Question Section included.
  DNSHeader *_dnsHeader = (DNSHeader *) msg.data();

  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->RCode = (unsigned char) replyCode;
  _dnsHeader->QDCount = 0;
  _dnsHeader->ARCount = 0;

  packet.send(msg);
}

String DNSServer::askServerForIp(String url) {
  int msgid = _coap->put(IPAddress(172, 16, 51, 161), 5688, "ip", "www.google.com");


//  WiFiClientSecure *client = new WiFiClientSecure;

  // // Ignore SSL certificate validation
  // client->setInsecure();

  // HTTPClient http;
  // String serverPath = "https://" + _upstream_doh + "/api/query";

  // http.begin(*client, serverPath);

  // http.addHeader("Content-Type", "application/json");

  // String httpRequestData = "{\"type\":\"A\",\"query\":\"";
  // httpRequestData += url;
  // httpRequestData += "\"}";

  // String payload = "{}";

  // int httpResponseCode = http.POST(httpRequestData);

  // if (httpResponseCode > 0) {
  //   if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY) {
  //     payload = http.getString();
  //   }
  // } else {
  //   DEBUG_OUTPUT.println(serverPath);
  //   DEBUG_OUTPUT.println(httpRequestData);
  //   DEBUG_OUTPUT.print("Error code: ");
  //   DEBUG_OUTPUT.println(httpResponseCode);
  //   return "";
  // }
  // http.end();

  // JSONVar res = JSON.parse(payload);

  // if (JSON.typeof(res) == "undefined") {
  //   DEBUG_OUTPUT.println(payload);
  //   DEBUG_OUTPUT.println("Parsing input failed!");
  //   return "";
  // }

  // if (strcmp(res["returnCode"], "NOERROR") != 0) {
  //   DEBUG_OUTPUT.print("there is an error =>");
  //   DEBUG_OUTPUT.println(payload);
  //   return "";
  // }

  // String result = res["response"];
  // String ip = getValueBetweenParentheses(result);

  // if (ip.length() < 4) {
  //   return "";
  // }

  // return ip;


  return "";

}


bool DNSServer::requestIncludesOnlyOneAQuestion(AsyncUDPPacket &packet, size_t _qnameLength) {
  unsigned char *_buffer = packet.data();
  DNSHeader *_dnsHeader = (DNSHeader *) _buffer;
  unsigned char *_startQname = _buffer + sizeof(DNSHeader);

  if (ntohs(_dnsHeader->QDCount) == 1 &&
      _dnsHeader->ANCount == 0 &&
      _dnsHeader->NSCount == 0) {
    // Test if we are dealing with a QTYPE== A
    u_int16_t qtype = *(_startQname + _qnameLength + 1); // we need to skip the closing label length
    if (qtype != 0x0001) { // Not an A type query
      return false;
    }
    if (_dnsHeader->ARCount == 0) {

      return true;
    } else if (ntohs(_dnsHeader->ARCount) == 1) {

      // test if the Additional Section RR is of type EDNS
      unsigned char *_startADSection =
              _startQname + _qnameLength + 4; //skipping the TYPE AND CLASS values of the Query Section
      // The EDNS pack must have a 0 lentght domain name followed by type 41
      if (*_startADSection != 0) //protocol violation for OPT record
      {
        return false;
      }
      _startADSection++;

      uint16_t *dnsType = (uint16_t *) _startADSection;

      if (ntohs(*dnsType) != 41) // something else than OPT/EDNS lives in the Additional section
      {
        return false;
      }

      return true;
    } else { // AR Count != 0 or 1
      return false;
    }
  } else { // QDcount != 1 || ANcount !=0 || NSCount !=0
    return false;
  }
}

void DNSServer::downCaseAndRemoveWwwPrefix(String &domainName) {
  domainName.toLowerCase();
  domainName.replace("www.", "");
}

String DNSServer::getValueBetweenParentheses(String str) {
  size_t start_index = str.indexOf("(") + 1;

  // Check if opening parenthesis is found
  if (start_index < 0) {
    return "";
  }

  size_t end_index = str.indexOf(")");

  // Check if closing parenthesis is found
  if (end_index < 0) {
    return "";
  }

  String value = str.substring(start_index, end_index);

  return value;
}


String DNSServer::getDomainNameWithoutWwwPrefix(unsigned char *start, size_t &_qnameLength) {
  String parsedDomainName = "";
  if (start == nullptr || *start == 0) {
    _qnameLength = 0;
    return parsedDomainName;
  }
  int pos = 0;
  while (true) {
    unsigned char labelLength = *(start + pos);
    for (int i = 0; i < labelLength; i++) {
      pos++;
      parsedDomainName += (char) *(start + pos);
    }
    pos++;
    if (pos > 254) {
      // failsafe, A DNAME may not be longer than 255 octets RFC1035 3.1
      _qnameLength = 1; // DNAME is a zero length byte
      return "";
    }
    if (*(start + pos) == 0) {
      _qnameLength = (size_t)(pos) + 1;  // We need to add the clossing label to the length
      downCaseAndRemoveWwwPrefix(parsedDomainName);

      return parsedDomainName;
    } else {
      parsedDomainName += ".";
    }
  }
}
