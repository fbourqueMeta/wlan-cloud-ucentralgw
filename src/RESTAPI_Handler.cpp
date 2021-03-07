//
// Created by stephane bourque on 2021-03-04.
//

#include <algorithm>
#include <iostream>
#include <iterator>

#include "RESTAPI_Handler.h"
#include "uAuthService.h"

#include "Poco/URI.h"

bool RESTAPIHandler::ParseBindings(const char *p,const char *r, BindingMap & bindings)
{
    std::string param,value;

    bindings.clear();

    while(*r)
    {
        if(*r == '{') {
            r++;
            while (*r != '}')
                param += *r++;
            r++;
            while (*p != '/' && *p)
                value += *p++;
            bindings[param] = value;
            value.clear();
            param.clear();
        } else if( *p != *r ) {
            return false;
        } else {
            r++;
            p++;
        }
    }

    return (*p == *r);
}

void RESTAPIHandler::PrintBindings() {
    for(auto &[key,value]:bindings_)
        std::cout << "Key = " << key << "  Value= " << value << std::endl;
}

void RESTAPIHandler::ParseParameters(HTTPServerRequest& request) {

    Poco::URI uri(request.getURI());
    parameters_ = uri.getQueryParameters();
}

static bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

uint64_t RESTAPIHandler::GetParameter(const std::string &Name,const uint64_t Default){

    for(const auto & i:parameters_)
    {
        if(i.first == Name) {
            if(!is_number(i.second))
                return Default;
            return std::stoi(i.second);
        }
    }
    return Default;
}

std::string RESTAPIHandler::GetParameter(const std::string &Name,const std::string & Default){
    for(const auto & i:parameters_)
    {
        if(i.first == Name)
            return i.second;
    }
    return Default;
}

const std::string & RESTAPIHandler::GetBinding(const std::string &Name, const std::string &Default) {
    auto E = bindings_.find(Name);

    if(E==bindings_.end())
        return Default;

    return E->second;
}

std::string RESTAPIHandler::RFC3339(uint64_t t)
{
    return Poco::DateTimeFormatter::format(Poco::DateTime(Poco::Timestamp::fromEpochTime(t)), Poco::DateTimeFormat::SORTABLE_FORMAT);
}

static std::string MakeList(const std::vector<std::string> & L)
{
    std::string Return;

    for(const auto i : L)
        if( Return.empty() )
            Return = i;
        else
            Return += ',' + i;

    return Return;
}

void RESTAPIHandler::ProcessOptions( HTTPServerResponse & Response )
{
    Response.setContentType("application/json");
    Response.setKeepAlive(true);
    Response.set("Access-Control-Allow-Origin", "*");
    Response.add("Access-Control-Allow-Headers", "*");
    Response.add("Access-Control-Allow-Method",MakeList(methods_));
    Response.send();
}

void RESTAPIHandler::PrepareResponse(HTTPServerResponse &Response,Poco::Net::HTTPResponse::HTTPStatus Status)
{
    Response.setStatus(Status);
    Response.setChunkedTransferEncoding(true);
    Response.setContentType("application/json");
    Response.set("Access-Control-Allow-Origin", "*");
    Response.add("Access-Control-Allow-Headers", "*");
    Response.add("Access-Control-Allow-Method",MakeList(methods_));
}

void RESTAPIHandler::BadRequest(HTTPServerResponse & Response) {
    PrepareResponse(Response, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    Response.send();
}

void RESTAPIHandler::UnAuthorized(HTTPServerResponse & Response )
{
    PrepareResponse(Response, Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
    Response.send();
}

void RESTAPIHandler::NotFound(HTTPServerResponse &Response) {
    PrepareResponse(Response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
    Response.send();
}

void RESTAPIHandler::OK(HTTPServerResponse &Response) {
    PrepareResponse(Response);
    Response.send();
}

bool RESTAPIHandler::ContinueProcessing( HTTPServerRequest & Request , HTTPServerResponse & Response )
{
    if(Request.getMethod() == Poco::Net::HTTPRequest::HTTP_OPTIONS)
    {
        ProcessOptions(Response);
        return false;
    } else if(std::find(methods_.begin(),methods_.end(),Request.getMethod()) == methods_.end())
    {
        BadRequest(Response);
        return false;
    }

    return true;
}

bool RESTAPIHandler::IsAuthorized(Poco::Net::HTTPServerRequest & Request, HTTPServerResponse & Response )
{
    if(uCentral::Auth::Service::instance()->IsAuthorized(Request))
    {
        return true;
    }
    else {
        UnAuthorized(Response);
    }
    return false;
}

void RESTAPIHandler::ReturnObject(Poco::JSON::Object & Object, HTTPServerResponse & Response) {
    PrepareResponse(Response);
    std::ostream & Answer = Response.send();
    Poco::JSON::Stringifier::stringify(Object, Answer);
}
