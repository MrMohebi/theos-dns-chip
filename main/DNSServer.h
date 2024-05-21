#ifndef DNSServer_h
#define DNSServer_h
#include <String.h>
#include <lwip/def.h>
#include <Arduino_JSON.h>
#include <AsyncUDP.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "coap-simple.h"


#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1
#define DNS_OPCODE_QUERY 0

#define MAX_QUEUE_SIZE 50

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


struct ResponseQueue{
  uint16_t id;
  AsyncUDPPacket *dnsPacket;
  char * resolvedIP;
} ;

class DNSServer
{
  public:
    DNSServer();
    void setTTL(const uint32_t &ttl);

    bool start(const uint16_t port, const String &upstream_doh);
    // stops the DNS server
    void stop();

    void checkToResponse();


  private:
    AsyncUDP _udp;
    uint32_t _ttl;
    DNSReplyCode _errorReplyCodeDefault;
    String _upstream_doh;
    Coap *_coap;
    ResponseQueue _queue[MAX_QUEUE_SIZE];

    bool requestIncludesOnlyOneAQuestion(AsyncUDPPacket &packet, size_t _qnameLength);
    String getDomainNameWithoutWwwPrefix(unsigned char *start, size_t & _qnameLength);
    String askServerForIp(String url);
    String getValueBetweenParentheses(String str);
    void downCaseAndRemoveWwwPrefix(String &domainName);
    void processRequest(AsyncUDPPacket &packet);
    void replyWithIP(AsyncUDPPacket &packet, IPAddress &resolvedIP, size_t &_qnameLength);
    void replyWithCustomCode(AsyncUDPPacket &packet, size_t &_qnameLength, DNSReplyCode replyCode = DNSReplyCode::NonExistentDomain);

};
#endif