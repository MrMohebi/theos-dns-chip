#ifndef DNSServer_h
#define DNSServer_h
#include <String.h>
#include <lwip/def.h>
#include <Arduino_JSON.h>
#include <AsyncUDP.h>
#include "coap-simple.h"
#include <vector>


#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1
#define DNS_OPCODE_QUERY 0

#define COAP_SERVER_PORT 5688


enum class DNSReplyCode : unsigned char
{
  NoError = 0,
  FormError = 1,
  ServerFailure = 2,
  NonExistentDomain = 3,
  NotImplemented = 4,
  Refused = 5,
  YXDomain = 6,
  YXRRSet = 7,
  NXRRSet = 8
};

struct DNSHeader
{
  uint16_t ID;               // identification number
  unsigned char RD : 1;      // recursion desired
  unsigned char TC : 1;      // truncated message
  unsigned char AA : 1;      // authority answer
  unsigned char OPCode : 4;  // message_type
  unsigned char QR : 1;      // query/response flag
  unsigned char RCode : 4;   // response code
  unsigned char Z : 3;       // its z! reserved
  unsigned char RA : 1;      // recursion available
  uint16_t QDCount;          // number of question entries
  uint16_t ANCount;          // number of answer entries
  uint16_t NSCount;          // number of authority entries
  uint16_t ARCount;          // number of resource entries
};


class ResponseQueue{
  public: 
    uint16_t id;
    IPAddress addr;
    uint16_t port;
    IPAddress resolvedIP;
    bool ipHasSet;
    AsyncUDPMessage* msg;
    size_t qnameLength;

    void setMessage(AsyncUDPMessage* message) {
        msg = message;
    }
};

class Responses {
  public:
   static inline  std::vector<ResponseQueue> queue;
};



class DNSServer
{
  public:
    DNSServer();
    void setTTL(const uint32_t &ttl);
    bool start(const uint16_t port, IPAddress &upstream_doh);
    void stop();
    void checkToResponse();
    void setCOAP(Coap *coap);


  private:
    AsyncUDP _udp;
    uint32_t _ttl;
    DNSReplyCode _errorReplyCodeDefault;
    IPAddress _upstream_doh;
    Coap *_coap;

    bool requestIncludesOnlyOneAQuestion(AsyncUDPPacket &packet, size_t _qnameLength);
    String getDomainNameWithoutWwwPrefix(unsigned char *start, size_t & _qnameLength);
    void askServerForIp(AsyncUDPPacket &packet,String url, size_t &_qnameLength);
    String getValueBetweenParentheses(String str);
    void downCaseAndRemoveWwwPrefix(String &domainName);
    void processRequest(AsyncUDPPacket &packet);
    void replyWithIP(ResponseQueue &responseQueue);

    void replyWithCustomCode(AsyncUDPPacket &packet, DNSReplyCode replyCode = DNSReplyCode::NonExistentDomain);
    void replyWithCustomCode(ResponseQueue &responseQueue, DNSReplyCode replyCode = DNSReplyCode::NonExistentDomain);

};
#endif