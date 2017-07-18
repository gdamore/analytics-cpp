//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <exception>
#include <map>
#include <string>
#include <vector>

#ifndef ANALYTICS_HPP
#define ANALYTICS_HPP

namespace segment {

class HttpError : public std::exception {

public:
    HttpError(int);
    const char* what() const throw();
    int HttpCode() const;

private:
    char buf[128];
    int code;
};

class SocketError : public std::exception {
public:
    SocketError();
    SocketError(int);
    const char* what() const throw();

private:
    int os_errno;
};

class Response {
public:
    Response();
    ~Response();

    bool ok;
    int status;
    std::string data;
    static size_t writeCallback(void*, size_t, size_t, void*);
};

typedef enum {
    EVENT_TYPE_IDENTIFY,
    EVENT_TYPE_TRACK,
    EVENT_TYPE_PAGE,
    EVENT_TYPE_SCREEN,
    EVENT_TYPE_GROUP,
    EVENT_TYPE_ALIAS
} EventType;

class Event {
public:
    Event(EventType type);
    ~Event();

    std::string Serialize();
    std::string Type();

    std::string userId;
    std::string event;
    std::string groupId;
    std::string anonymousId;
    std::string previousId;
    std::map<std::string, std::string> properties;

private:
    EventType type;
};

class HttpRequest {
public:
    // Method should be GET, POST, or similar.  At present only POST is
    // supported.
    std::string Method;

    // URL represents the entire URL, including the Host and the Path.
    std::string URL;

    // Note that if you want to have multiple values for the same map key,
    // you have to use comma seperated white space values.  The HTTP RFCs
    // insist that this must be permitted, and must not change any
    // semantics.  Demanding that our consumers use this semantic means
    // that we can give them a nice familiar API (STL maps).
    std::map<std::string, std::string> Headers;

    // The request body is here.  Normally this will be something like
    // serialized JSON.  This will generally be reasonably small, and
    // never larger than about 512k as Segment prohibits uploading
    // more data than that in a single POST.
    std::vector<char> Body;
};

class HttpResponse {
public:
    // Code is an HTTP response code, like 200 or 404.  If the request
    // could not be delivered to the server, or a valid response could
    // not be obtained, then the code 0 should be used, and the Message
    // below should be completed with an error message.
    int Code;

    // Message is a message explaining the result.  Normally this comes
    // directly from the HTTP server, but it can also be a system
    // error message (e.g. "Host not found" or "No route to destination" or
    // even "Out of memory").
    std::string Message;

    /// Headers are the response headers we got back from the server.
    // Nothing in the current code actually cares abotu this.
    std::map<std::string, std::string> Headers;

    // Body is the response body, if a payload was returned.
    // Nothing actually uses this at present, and at present the
    // Segment API returns an empty body anyway.
    std::vector<char> Body;
};

// HttpHandler is an abstract class for handling HTTP.  Note that
// at present we only strictly require the ability to POST, and we
// don't really care if the implementation bothers to flesh out the
// Body of the response.  Nonetheless, it is recommended that
// implementations do fill this out to future-proof in case we
// begin using more fully featured handles.
class HttpHandler {
public:
    virtual ~HttpHandler();
    // Handle taks a request and turns it into a response, generally
    // by posting it to the Segment API service.  This API is strictly
    // synchronous, but Segment always performs this operation from
    // a context where this is safe.  (XXX: This may not be true today,
    // but it will be when we start performing batched requests.)
    virtual void Handle(const HttpRequest&, HttpResponse&) = 0;
};

class Analytics {
public:
    Analytics(std::string writeKey);
    Analytics(std::string writeKey, std::string host);
    ~Analytics();

    // Flush flushes events to the server.  If wait is true, then
    // the caller will block until all queued events are fully uploaded
    // to the server (or a failure uploading them was encountered).
    // Flush should be called with wait true before exiting the
    // application to ensure that all events of interest are uploaded.
    void Flush(bool wait);

    // SetHandler allows for a different handler object to be registered.
    // In this case the Analytics object will route HTTP requests through
    // the function.
    void SetHandler(std::shared_ptr<HttpHandler>);

    // DefaultHandler returns the default handler.  Normally this is
    // just a reference to a static implementation based on libcurl,
    // but if this library was compiled without CURL support it may
    // be a function that does nothing.
    std::shared_ptr<HttpHandler> Handler();

    void Track(std::string userId, std::string event, std::map<std::string, std::string> properties);
    void Track(std::string userId, std::string event);

    void Identify(std::string userId, std::map<std::string, std::string> traits);
    void Identify(std::string userId);

    void Page(std::string event, std::string userId, std::map<std::string, std::string> properties);
    void Page(std::string event, std::string userId);

    void Screen(std::string event, std::string userId, std::map<std::string, std::string> properties);
    void Screen(std::string event, std::string userId);

    void Alias(std::string previousId, std::string userId);

    void Group(std::string groupId, std::map<std::string, std::string> properties);
    void Group(std::string groupId);

private:
    std::string writeKey;
    std::string host;
    std::string userUage;
    std::shared_ptr<HttpHandler> handler;

    void sendEvent(Event& e);
};

} // namespace segment

#endif // ANALYTICS_HPP