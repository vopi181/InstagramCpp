#include "InstagramClient.h"
#include "InstagramParsers.h"
#include "InstagramConstants.h"
#include "InstagramEndpoints.h"
#include "FormData.h"

#include "HttpUrl.h"
#include "HttpResponse.h"

namespace Instagram {

inline Http::HttpUrl getUrl(const std::string &str){
    return {INSTAGRAM_HOST, str, Http::HttpProtocol::HTTPS};
}

InstagramClient::InstagramClient() :  m_httpClient {}, m_authToken { "" } {}

InstagramClient::InstagramClient(InstagramClient&& client) : InstagramClient{} {
    swap(*this, client);
}

InstagramClient::InstagramClient(const std::string& authToken) : m_httpClient{}, m_authToken{authToken}{}

InstagramClient& InstagramClient::operator=(InstagramClient&& client){
    m_httpClient = std::move(client.m_httpClient);
    m_authToken = std::move(client.m_authToken);

    return *this;
}

const std::string& InstagramClient::getAuthToken() const {
    return m_authToken;
}

void InstagramClient::setAuthToken(const std::string& authToken) {
    m_authToken = authToken;
}

inline bool InstagramClient::checkAuth() const {
    return !m_authToken.empty();
}

std::string InstagramClient::getResult(const Http::HttpResponse& response) const {
    switch (response.code()) {
    case Http::Status::BAD_REQUEST:
        return getError(response.body());
    default:
        return response.getString();
    }
}

AuthorizationToken InstagramClient::authenticate(const std::string& code,
        const std::string& clientId,
        const std::string& clientSecret,
        const std::string& redirectUri) {

    Http::FormData form_data {FORM_DATA_BOUNDARY};
    form_data["code"] = code;
    form_data["client_id"] = clientId;
    form_data["client_secret"] = clientSecret;
    form_data["redirect_uri"] = redirectUri;
    form_data["grant_type"] = AUTH_CODE_GRANT_TYPE;

    const Http::HttpResponse response = m_httpClient.post(getUrl(Auth::GET_AUTH_CODE), form_data);
    if (response.code() == Http::Status::OK) {
        AuthorizationToken authToken = parseAuthToken(response.body());
        setAuthToken(authToken.token());

        return authToken;
    } else {
        return getResult(response);
    }
}

UserInfo InstagramClient::getUserInfo() const {
    return getUserInfo(SELF);
}

UserInfo InstagramClient::getUserInfo(const std::string& userId) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Users::users + userId);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseUserInfo(response.body());
    } else {
        return getResult(response);
    }
}

MediaEntries InstagramClient::getRecentMedia(unsigned count) const {
    return getRecentMedia(SELF, count);
}

MediaEntries InstagramClient::getRecentMedia(const std::string& userId, unsigned count) const {
    if(!checkAuth()){ 
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Users::users + userId + Media::recentMedia);
    url[AUTH_TOKEN_ARG] = m_authToken;
    url[COUNT_ARG] = std::to_string(count);

    return getMedia(url);
} 

MediaEntries InstagramClient::getRecentMedia(const std::string& min_id, const std::string& max_id, unsigned count) const {
    return getRecentMedia(SELF, min_id, max_id, count);
}

MediaEntries InstagramClient::getRecentMedia(const std::string& userId, const std::string& min_id, const std::string& max_id, unsigned count) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Users::users + userId + Media::recentMedia);
    url[AUTH_TOKEN_ARG] = m_authToken;
    
    if (!min_id.empty()) url[MIN_ID_ARG] = min_id;
    if (!max_id.empty()) url[MAX_ID_ARG] = max_id;
    
    url[COUNT_ARG] = std::to_string(count);

    return getMedia(url);
}

MediaEntries InstagramClient::getLikedMedia(unsigned int count) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Users::users} + Users::ownLikes);
    url[AUTH_TOKEN_ARG] = m_authToken;
    url[COUNT_ARG] = std::to_string(count);

    return getMedia(url);
}

MediaEntries InstagramClient::getLikedMedia(const std::string& max_id, unsigned int count) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Users::users} + Users::ownLikes);
    url[AUTH_TOKEN_ARG] = m_authToken;
    url[MAX_LIKE_ID] = max_id;
    url[COUNT_ARG] = std::to_string(count);

    return getMedia(url);
}

MediaEntries InstagramClient::getMedia(const Http::HttpUrl& url) const {
    const Http::HttpResponse response = m_httpClient << url;

    if (response.code() == Http::Status::OK) {
        return parseMediaEntries(response.body());
    } else {
        return getResult(response);
    }
}

UsersInfo InstagramClient::searchUsers(const std::string& query, unsigned count) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Users::users} + Users::search);
    url[AUTH_TOKEN_ARG] = m_authToken;
    url[QUERY_ARG] = query;
    url[COUNT_ARG] = std::to_string(count);

    return getUsersInfo(url);
}

UsersInfo InstagramClient::getFollows() const {
    if(!checkAuth()){
       return NOT_AUTHENTICATED;
    } 
    Http::HttpUrl url = getUrl(std::string{Users::users} + Relationships::follows);
    url[AUTH_TOKEN_ARG] = m_authToken;
    return getUsersInfo(url);
}

UsersInfo InstagramClient::getFollowedBy() const {
    if(!checkAuth()){
       return NOT_AUTHENTICATED;
    } 

    Http::HttpUrl url = getUrl(std::string{Users::users} + Relationships::followedBy);
    url[AUTH_TOKEN_ARG] = m_authToken;
    return getUsersInfo(url);
}

UsersInfo InstagramClient::getRequestedBy() const {
    if(!checkAuth()){
       return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Users::users} + Relationships::requestedBy);
    url[AUTH_TOKEN_ARG] = m_authToken;
    return getUsersInfo(url);
}

UsersInfo InstagramClient::getUsersInfo(const Http::HttpUrl& url) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    } 

    const Http::HttpResponse response = m_httpClient << url;

    if (response.code() == Http::Status::OK) {
        return parseUsersInfo(response.body());
    } else {
        return getResult(response);
    }
}

RelationshipInfo InstagramClient::getRelationshipInfo(const std::string& userId) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Users::users + userId + Relationships::relationship);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;

    if (response.code() == Http::Status::OK) {
        return parseRelationshipInfo(response.body());
    } else {
        return getResult(response);
    }
}

RelationshipInfo InstagramClient::follow(const std::string& userId){
    return postRelationship(Relationship::follow, userId);
}

RelationshipInfo InstagramClient::unfollow(const std::string& userId){
    return postRelationship(Relationship::unfollow, userId);
}

RelationshipInfo InstagramClient::approve(const std::string& userId){
    return postRelationship(Relationship::approve, userId);
}

RelationshipInfo InstagramClient::ignore(const std::string& userId){
    return postRelationship(Relationship::ignore, userId);
}

RelationshipInfo InstagramClient::postRelationship(Relationship relationship, const std::string& userId){
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Users::users + userId + Relationships::relationship);
    Http::FormData formData{FORM_DATA_BOUNDARY};

    switch(relationship){
        case Relationship::follow:
            formData[RELATIONSHIP_ACTION_ARG] = RELATIONSHIP_FOLLOW;
            break;
        case Relationship::unfollow:
            formData[RELATIONSHIP_ACTION_ARG] = RELATIONSHIP_UNFOLLOW;
            break;
        case Relationship::approve:
            formData[RELATIONSHIP_ACTION_ARG] = RELATIONSHIP_APPROVE;
            break;
        case Relationship::ignore:
            formData[RELATIONSHIP_ACTION_ARG] = RELATIONSHIP_IGNORE;
            break;
    }
    formData[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient.post(url, formData);
    if(response.code() == Http::Status::OK){
        return parseRelationshipInfo(response.body());
    }else{
        return getResult(response);
    }
}

MediaEntry InstagramClient::getMedia(const std::string& mediaId) const {
    if(!checkAuth()){ 
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url  = getUrl(Media::media + mediaId);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseMediaEntry(response.body());
    } else {
        return getResult(response);
    }
}

MediaEntry InstagramClient::getMediaWithShortCode(const std::string& shortcode) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    } 

    Http::HttpUrl url = getUrl(Media::byShortCode + shortcode);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseMediaEntry(response.body());
    } else {
        return getResult(response);
    }
}

MediaEntries InstagramClient::searchMedia(double lat, double lng, int distance) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    checkAuth();

    Http::HttpUrl url = getUrl(Media::search);
    url[LAT_ARG] = std::to_string(lat);
    url[LNG_ARG] = std::to_string(lng);
    url[DST_ARG] = std::to_string(distance);
    url[AUTH_TOKEN_ARG] = m_authToken;

    return getMedia(url);
}

CommentsInfo InstagramClient::getComments(const std::string& mediaId) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Comments::comments);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseComments(response.body());
    } else {
        return getResult(response);
    }
}

BaseResult InstagramClient::comment(const std::string& mediaId, const std::string& text) {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Comments::comments);
    url[AUTH_TOKEN_ARG] = m_authToken;

    Http::FormData form_data {FORM_DATA_BOUNDARY};
    form_data[Comments::TEXT_ARG] = text;

    const Http::HttpResponse response = m_httpClient.post(url, form_data);
    if (response.code() == Http::Status::OK) {
        return {};
    } else {
        return getResult(response);
    }
}

BaseResult InstagramClient::deleteComment(const std::string& mediaId, const std::string& commentId) {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Comments::comments + commentId);
    url[AUTH_TOKEN_ARG] = m_authToken;
    const Http::HttpResponse response = m_httpClient.del(url);
    if (response.code() == Http::Status::OK) {
        return {};
    } else {
        return getResult(response);
    }
}

UsersInfo InstagramClient::getLikes(const std::string& mediaId) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Likes::likes);
    url[AUTH_TOKEN_ARG] = m_authToken;

    return getUsersInfo(url);
}

BaseResult InstagramClient::like(const std::string& mediaId) {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Likes::likes);
    Http::FormData form_data {FORM_DATA_BOUNDARY};
    form_data[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient.post(url, form_data);
    if (response.code() == Http::Status::OK) {
        return {};
    } else {
        return getResult(response);
    }
}

BaseResult InstagramClient::unlike(const std::string& mediaId) {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Media::media + mediaId + Likes::likes);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient.del(url);
    if (response.code() == Http::Status::OK) {
        return {};
    } else {
        return getResult(response);
    }
}

TagInfo InstagramClient::getTagInfo(const std::string& tag_name) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Tags::tags + tag_name);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseTagInfo(response.body());
    } else {
        return getResult(response);
    }
}

TagsInfo InstagramClient::searchTags(const std::string& query) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Tags::tags} + Tags::tagsSearch);
    url[QUERY_ARG] = query;
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseTagsInfo(response.body());
    } else {
        return getResult(response);
    }

}

MediaEntries InstagramClient::getRecentMediaForTag(const std::string& tag_name) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Tags::tags + tag_name + Media::recentMedia);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseMediaEntries(response.body());
    } else {
        return getResult(response);
    }
}

LocationInfo InstagramClient::getLocationById(const std::string& location_id) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Locations::locations + location_id);
    url[AUTH_TOKEN_ARG] = m_authToken;

    const Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseLocation(response.body());
    } else {
        return getResult(response);
    }
}

MediaEntries InstagramClient::getMediaForLocation(const std::string& location_id) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(Locations::locations + location_id + Media::recentMedia);
    Http::HttpResponse response = m_httpClient << url;

    if (response.code() == Http::Status::OK) {
        return parseMediaEntries(response.body());
    } else {
        return getResult(response);
    }
}

LocationsInfo InstagramClient::searchLocations(double lat, double lng, int distance) const {
    if(!checkAuth()){
        return NOT_AUTHENTICATED;
    }

    Http::HttpUrl url = getUrl(std::string{Locations::locations} + Locations::search);
    url[LAT_ARG] = std::to_string(lat);
    url[LNG_ARG] = std::to_string(lng);
    url[DST_ARG] = std::to_string(distance);
    url[AUTH_TOKEN_ARG] = m_authToken;

    Http::HttpResponse response = m_httpClient << url;
    if (response.code() == Http::Status::OK) {
        return parseLocations(response.body());
    } else {
        return getResult(response);
    }
}

void swap(InstagramClient& first, InstagramClient& second){
    using std::swap;
    swap(first.m_httpClient, second.m_httpClient);
    swap(first.m_authToken, second.m_authToken);
}

}
