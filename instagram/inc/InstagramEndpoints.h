//
// Created by inside on 4/6/16.
//

#ifndef HTTPSERVER_INSTAGRAMMETHODS_H
#define HTTPSERVER_INSTAGRAMMETHODS_H

namespace Instagram{
    static const char* GET_AUTH_CODE = "/oauth/access_token";
    static const char* self = "self";
    
    namespace Users{
        static const char* users = "/v1/users/";
        
        static const char* query_arg = "q";

        static const char* recent_media = "media/recent";
        static const char* search = "search";
        static const char* self_liked = "self/media/liked";
    }

    namespace Relationships{
        static const char* follows  = "/v1/users/self/follows";
        static const char* followed_by = "/v1/users/self/followed-by";
        static const char* requested_by = "/v1/users/self/requested-by";
    }
}

#endif //HTTPSERVER_INSTAGRAMMETHODS_H
