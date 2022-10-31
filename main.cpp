#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#include <list>
#include <thread>
#include <mutex>

std::mutex m;

std::list<int> clients_soc;
void send_message_for_all(int clientSocket, char *buf, int bytesRecv){
    std::lock_guard<std::mutex> lk(m);
    int x=0;
    for(auto c:clients_soc){
        if(c != clientSocket){
            while(x != bytesRecv+1){
                int result_send;
                result_send = send(c, buf, bytesRecv+1, 0);
                if(result_send > 0){
                    x += result_send;
                }
            }
        }
    }
}

void remove_client_from_list(int clientSocket){
    std::lock_guard<std::mutex> lk(m);
    clients_soc.remove(clientSocket);
}

void print_sockets(){
    std::lock_guard<std::mutex> lk(m);
    for(auto c:clients_soc){
        std::cout<< c << " ";
    }
    std::cout<<std::endl;
}

void client_handler(int clientSocket)
{
    char buf[4096];
    while (true) {
        // clear buffer
        memset(buf, 0, 4096);

        // wait for a message
        int bytesRecv = recv(clientSocket, buf, 4096, 0);
        if (bytesRecv == -1)
        {
            std::cerr << "There was a connection issue." << std::endl;
            break;
        }
        if (bytesRecv == 0)
        {
            std::cout << "The client disconnected" << std::endl;
            remove_client_from_list(clientSocket);
            print_sockets();
            break;
        }  
        // display message
        std::cout << "Received: " << std::string(buf, 0, bytesRecv);

        //send message
        send_message_for_all( clientSocket, buf, bytesRecv );

    }
    // close socket
    close(clientSocket);
}

int main()
{
    
    std::cout << "Creating server socket..." << std::endl;
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1){
        std::cerr << "Can't create a socket!";
        return -1;
    }

    struct sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(8000);
    inet_pton(AF_INET, "127.0.0.0", &hint.sin_addr);

    std::cout << "Binding socket to sockaddr..." << std::endl;
    if (bind(listening, (struct sockaddr *)&hint, sizeof(hint)) == -1) {
        std::cerr << "Can't bind to IP/port";
        return -2;
    }

    std::cout << "Mark the socket for listening..." << std::endl;
    if (listen(listening, SOMAXCONN) == -1){
        std::cerr << "Can't listen !";
        return -3;
    }

    std::list<std::thread> threads;

    while(true){
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        std::cout << "Accept client call..." << std::endl;
        int clientSocket = accept(listening, (struct sockaddr *)&client, &clientSize);

        std::cout << "Received call..." << std::endl;
        if (clientSocket == -1){
            std::cerr << "Problem with client connecting!";
            break;
        }

        {
            std::lock_guard<std::mutex> lk(m);
            clients_soc.push_back(clientSocket);
        }

        std::cout << "Client address: " << inet_ntoa(client.sin_addr) << " and port: " << client.sin_port << std::endl;
        threads.emplace_back(client_handler, clientSocket);
    }
    close(listening);

    for (auto& t: threads){
        t.join();
    }
    threads.clear();

    return 0;
}