
#include "Client.hpp"

Client::~Client() {
}

Client::Client() : _nickname(std::string("*")), _isRegistered(false){
}

Client::Client(Client const & other) {
	_nickname = other._nickname;
	_channels = other._channels;
	_rights = other._rights;
	_realname = other._realname;
	_tmpPassword = other._tmpPassword;
	_tmpRequest = other._tmpRequest;
	_operName = other._operName;
	_isRegistered = other._isRegistered;
}

void	Client::setTmpPwd(std::string tmpPwd) {
    _tmpPassword = tmpPwd;
}
void	Client::setNickname(std::string nickname) {
    _nickname = nickname;
}
void	Client::setClientname(std::string Clientname) {
    _username = Clientname;
}
void	Client::setRealname(std::string realname) {
    _realname = realname;
}
void	Client::setOperName(std::string opername) {
    _operName = opername;
}
void	Client::appendTmpRequest(std::string request) {
    _tmpRequest.append(request);
}
bool	Client::isRegistered() {
    return _isRegistered;
}

std::string const & Client::getClientname() const {
    return _username;
}
std::string const & Client::getRealName() const {
    return _realname;
}
std::string const & Client::getNickname() const {
    return _nickname;
}
bool const & Client::getRegistered() const {
    return _isRegistered;
}

std::string & Client::getTmpPwd()  {
    return _tmpPassword;
}
std::string & Client::getTmpRequest() {
    return _tmpRequest;
}
std::string const & Client::getOperName()  const {
    return _operName;
}

void	Client::setRegistered(int val) {
    if (val)
        _isRegistered = true;
    else
        _isRegistered = false;
}
void		Client::cleanTmpRequest(){
    _tmpRequest.clear();
}