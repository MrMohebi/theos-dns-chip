#include "./DNSServer.h"



#define DEBUG
#define DEBUG_OUTPUT Serial

DNSServer::DNSServer()
{
  _ttl = htonl(60);
  _errorReplyCode = DNSReplyCode::NonExistentDomain;
}

bool DNSServer::start(const uint16_t &port, const String &upstream_doh )
{
  _port = port;
  _upstream_doh = upstream_doh;
  // _domainName = domainName;
  // _resolvedIP[0] = resolvedIP[0];
  // _resolvedIP[1] = resolvedIP[1];
  // _resolvedIP[2] = resolvedIP[2];
  // _resolvedIP[3] = resolvedIP[3];
  // downcaseAndRemoveWwwPrefix(_domainName);
  return _udp.begin(_port) == 1;
}

void DNSServer::setErrorReplyCode(const DNSReplyCode &replyCode)
{
  _errorReplyCode = replyCode;
}

void DNSServer::setTTL(const uint32_t &ttl)
{
  _ttl = htonl(ttl);
}

void DNSServer::stop()
{
  _udp.stop();
}

void DNSServer::downcaseAndRemoveWwwPrefix(String &domainName)
{
  domainName.toLowerCase();
  domainName.replace("www.", "");
}

void DNSServer::processNextRequest()
{
  _currentPacketSize = _udp.parsePacket();
  if (_currentPacketSize)
  {
    DEBUG_OUTPUT.println("got new udp");

    _buffer = (unsigned char*)malloc(_currentPacketSize * sizeof(char));
    _udp.read(_buffer, _currentPacketSize);
    _dnsHeader = (DNSHeader*) _buffer;

    if (_dnsHeader->QR == DNS_QR_QUERY &&
        _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
        requestIncludesOnlyOneQuestion() &&
        (_domainName == "*" || getDomainNameWithoutWwwPrefix() == _domainName)
       )
    {
      replyWithIP();
    }
    else if (_dnsHeader->QR == DNS_QR_QUERY)
    {
      replyWithCustomCode();
    }

    free(_buffer);
  }
}

bool DNSServer::requestIncludesOnlyOneQuestion()
{
  return ntohs(_dnsHeader->QDCount) == 1 &&
         _dnsHeader->ANCount == 0 &&
         _dnsHeader->NSCount == 0 &&
         _dnsHeader->ARCount == 0;
}

String DNSServer::getDomainNameWithoutWwwPrefix()
{
  String parsedDomainName = "";
  unsigned char *start = _buffer + 12;
  if (*start == 0)
  {
    return parsedDomainName;
  }
  int pos = 0;
  while(true)
  {
    unsigned char labelLength = *(start + pos);
    for(int i = 0; i < labelLength; i++)
    {
      pos++;
      parsedDomainName += (char)*(start + pos);
    }
    pos++;
    if (*(start + pos) == 0)
    {
      downcaseAndRemoveWwwPrefix(parsedDomainName);
      return parsedDomainName;
    }
    else
    {
      parsedDomainName += ".";
    }
  }
}

String DNSServer::getValueBetweenParentheses(String str){
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

String DNSServer::askServerForIp(String url){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  // Ignore SSL certificate validation
  client->setInsecure();

  HTTPClient http;
  String serverPath = "https://" + _upstream_doh + "/api/query";

  http.begin(*client, serverPath);
  
  http.addHeader("Content-Type", "application/json");

  String httpRequestData = "{\"type\":\"A\",\"query\":\"";
  httpRequestData += url;
  httpRequestData += "\"}";

  String payload = "{}"; 

  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode>0) {
    if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY) {
          payload = http.getString();
    }
  }
  else {
    DEBUG_OUTPUT.println(serverPath);
    DEBUG_OUTPUT.println(httpRequestData);
    DEBUG_OUTPUT.print("Error code: ");
    DEBUG_OUTPUT.println(httpResponseCode);
    return "";
  }
  http.end();

  JSONVar res = JSON.parse(payload);

  if (JSON.typeof(res) == "undefined") {
    DEBUG_OUTPUT.println(payload);
    DEBUG_OUTPUT.println("Parsing input failed!");
    return "";
  }

  if (strcmp(res["returnCode"], "NOERROR") != 0){
    DEBUG_OUTPUT.print("there is an error =>");
    DEBUG_OUTPUT.println(payload);
    return "";
  }

  String result = res["response"];
  String ip = getValueBetweenParentheses(result);

  if(ip.length() < 4){
    return "";
  }

  return ip;
}



void DNSServer::replyWithIP()
{
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->ANCount = _dnsHeader->QDCount;
  _dnsHeader->QDCount = _dnsHeader->QDCount; 
  //_dnsHeader->RA = 1;  

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, _currentPacketSize);

  _udp.write((uint8_t)192); //  answer name is a pointer
  _udp.write((uint8_t)12);  // pointer to offset at 0x00c

  _udp.write((uint8_t)0);   // 0x0001  answer is type A query (host address)
  _udp.write((uint8_t)1);

  _udp.write((uint8_t)0);   //0x0001 answer is class IN (internet address)
  _udp.write((uint8_t)1);
 
  _udp.write((unsigned char*)&_ttl, 4);

  // Length of RData is 4 bytes (because, in this case, RData is IPv4)
  _udp.write((uint8_t)0);
  _udp.write((uint8_t)4);
  _udp.write(_resolvedIP, sizeof(_resolvedIP));
  _udp.endPacket();



  #ifdef DEBUG
    DEBUG_OUTPUT.print("DNS responds: ");
    DEBUG_OUTPUT.print(_resolvedIP[0]);
    DEBUG_OUTPUT.print(".");
    DEBUG_OUTPUT.print(_resolvedIP[1]);
    DEBUG_OUTPUT.print(".");
    DEBUG_OUTPUT.print(_resolvedIP[2]);
    DEBUG_OUTPUT.print(".");
    DEBUG_OUTPUT.print(_resolvedIP[3]);
    DEBUG_OUTPUT.print(" for ");
    DEBUG_OUTPUT.println(getDomainNameWithoutWwwPrefix());
  #endif
}

void DNSServer::replyWithCustomCode()
{
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->RCode = (unsigned char)_errorReplyCode;
  _dnsHeader->QDCount = 0;

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, sizeof(DNSHeader));
  _udp.endPacket();
}