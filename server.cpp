#include<bits/stdc++.h>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <header.h>
using boost::asio::ip::tcp;
using namespace std;
const int PORT = 1234;
int BIDDERS=0;
int PRICES=0;

int32_t main(int32_t argv,char *argc[]) {
    BIDDERS=stoi(argc[1]);
    PRICES=stoi(argc[2]);
    vector<vector<int>> b(BIDDERS,vector<int>(PRICES,0));
    vector<int> B(PRICES,0);
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));

        std::vector<tcp::socket> bidders;

        std::cout << "Server listening on port " << PORT << "...\n";

        for (int i = 0; i < BIDDERS; ++i) {  
            tcp::socket socket(io_context);
            acceptor.accept(socket);
 
            vector<int> temp(PRICES,0);
            boost::asio::read(socket, boost::asio::buffer(temp.data(), PRICES*sizeof(int)));
            b[i]=temp;
            socket.close();
        }
        for(int j=0;j<PRICES;j++){
            for(int i=0;i<BIDDERS;i++){
                B[j]+=b[i][j];
                B[j]%=MOD;
            }
        }
        bool found=false;int winnning_price;
        struct winner result;
        for(int i=PRICES-1;i>=0;i--){
            if(B[i]){
                winnning_price=i;found=true;
                result.price=i;
                result.value=B[i];
                break;
            }
        }
        for(int i=0;i<BIDDERS;i++){
            int port=12345+i;
            tcp::socket socket(io_context);
            socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
            boost::asio::write(socket, boost::asio::buffer(&result, sizeof(winner)));
            socket.close();
        }
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
    return 0;
}

