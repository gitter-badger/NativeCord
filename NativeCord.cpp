//
// Created by wolverindev on 28.08.16.
//

#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "cpr/cpr.h"
#include "include/utils/SocketUtil.h"
#include "include/config/Configuration.h"
#include "include/connection/PlayerConnection.h"
#include "include/plugin/PluginManager.h"
#include "include/plugin/java/JavaPluginManagerImpl.h"
#include "include/log/Terminal.h"
#include "NativeCord.h"

using namespace std;

void error(const char* message){
    cerr << message << endl;
    exit(-1);
}

int ssockfd = 0;
sockaddr_in* cli_addr = nullptr;

void shutdownHook(void){
    cout << "Closing socket" << endl;
    close(ssockfd);
    if(cli_addr != nullptr)
        delete cli_addr;
}

void clientConnect(){
    try{
        ssockfd = SocketUtil::createTCPServerSocket(Configuration::instance->config["network"]["port"].as<int>());
        if(ssockfd < 0){
            ssockfd = SocketUtil::createTCPServerSocket(Configuration::instance->config["network"]["port"].as<int>()+1); //TEST MODE!
            if(ssockfd < 0){
                error("Cant create socket.");
            }
        }
        while (1) {
            cli_addr = new sockaddr_in();
            socklen_t clilen = sizeof(*cli_addr);
            int newsockfd = accept(ssockfd, (struct sockaddr *) cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            Socket *connection = new Socket(newsockfd);
            PlayerConnection *playerConnection = new PlayerConnection(cli_addr ,connection);
            playerConnection->start();
            //pthread_join((handler->getThreadHandle()),NULL);
        }
    }catch (Exception* e){
        cout << "Exception message: " << e->what() << endl;
    }
}

pthread_t NativeCord::clientAcceptThread;

void NativeCord::exitNativeCoord(){
    PlayerConnection::connections.clear();
    PlayerConnection::activeConnections.clear();
    pthread_cancel(NativeCord::clientAcceptThread);
    pthread_join(NativeCord::clientAcceptThread, NULL);
    ServerInfo::reset();

    debugMessage("Buffers: " + to_string(DataBuffer::creations));
    debugMessage("Chat instances: " + to_string(ChatMessage::count));

    JavaPluginManagerImpl::instance->disable(); // Close the process
}

int preloadV(){
    Terminal::setupTerminal();
    Terminal::instance = new Terminal();
    Terminal::instance->startReader();
    return 0;
}
int preload = preloadV();

int main(int argc, char** argv) {
    logMessage("Hello world");

    JavaPluginManagerImpl* manager = new JavaPluginManagerImpl();
    if(!manager->enable()){
        logError("Cant enable plugin manager!");
        return 0;
    }

    /*
    DataStorage* handle = new DataStorage;
    handle->longs.push_back(2222);
    handle->ints.push_back(123);
    handle->bytes.push_back(125);
    handle->floats.push_back(1.23);
    handle->doubles.push_back(3.21);
    handle->strings.push_back("Hello world");
    jobject  jobj = manager->storageImpl->toJavaObject(*handle);

    DataStorage* out = manager->storageImpl->fromJavaObject(jobj);
    cout << out->_toString() << endl;
    */

    //TODO load plugin right
    if(manager->getLoadedPlugins().size() > 0) {
        manager->enablePlugin(manager->getLoadedPlugins()[0]);
        DataStorage storage;
        manager->callEvent(EventType::PLAYER_HANDSCHAKE_EVENT, &storage);
    }
    //manager->disable();
    //delete manager;

    try {
        string filename = string("config.yml");
        Configuration::instance = new Configuration(filename);
        Configuration::instance->loadConfig();
        cout << "Loading configuration" << endl;
        if(!Configuration::instance->isValid()){
            logFatal("Configuration not valid!");
            vector<string> errors = Configuration::instance->getErrors();
            if(errors.size() == 1)
                logError("Error:");
            else
                logError("Errors (" + to_string(errors.size()) + "):");
            for(vector<string>::iterator it = errors.begin();it!=errors.end();it++){
                logError((errors.size() != 1 ? " - " : "") + *it);
            }
            return 0;
        }
        ServerInfo::loadServers();
        logMessage("Configuration successfull loaded.");
        pthread_create(&(NativeCord::clientAcceptThread),NULL,(void* (*)(void*)) &clientConnect,NULL);
        //pthread_join(clientAcceptThread,NULL);

        while (1){
            sleep(1000);
        }
    }catch(Exception* ex){
        logFatal("Having error on main thread: "+string(ex->what()));
        logMessage("Exiting");
        return 1;
    }
    return 0;
}