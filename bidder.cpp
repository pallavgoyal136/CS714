#include<bits/stdc++.h>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <header.h>
#include <iomanip>
using boost::asio::ip::tcp;
using namespace std;
random_device rd;
mt19937 gen(rd()); 
uniform_int_distribution<> distr(0, MOD);
int BIDDERS;
int PRICES;
double time_to_read=0,time_to_write=0;
std::mutex time_mutex;
void start_server(int my_port, vector<vector<int>>& received_values) {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), my_port));
        for(int i=0;i<BIDDERS-1;i++){
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            vector<int> temp(PRICES+1,0);
            auto transfer_start = std::chrono::steady_clock::now();
            boost::asio::read(socket, boost::asio::buffer(temp.data(),(PRICES+1)*sizeof(int)));
            auto transfer_end = std::chrono::steady_clock::now();
            double transfer_duration = std::chrono::duration<double>(transfer_end - transfer_start).count();
            time_to_read+=transfer_duration;
            int sender_port=temp.back();
            temp.pop_back();
            sender_port-=12345;
            received_values[sender_port]=temp;
            socket.close();
        }
    } catch (std::exception& e) {
        std::cerr << "Server error at port " << my_port << ": " << e.what() << "\n";
    }
}
void add_write_time(double transfer_duration){
    std::lock_guard<std::mutex> lock(time_mutex);
    time_to_write += transfer_duration;
    return;
}
void connect_to_bidder(int port, vector<int> &value_to_send,int sender_port) {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        value_to_send.push_back(sender_port);
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        auto transfer_start = std::chrono::steady_clock::now();
        boost::asio::write(socket, boost::asio::buffer(value_to_send.data(), (PRICES + 1) * sizeof(int)));
        auto transfer_end = std::chrono::steady_clock::now();
        double transfer_duration = std::chrono::duration<double>(transfer_end - transfer_start).count();
        add_write_time(transfer_duration);
        value_to_send.pop_back();
        socket.close();
    } catch (std::exception& e) {
        std::cerr << "Error connecting to bidder at port " << port << ": " << e.what() << "\n";
    }
}
void save_to_file(double time_select,int my_index) {
    std::string directory = "N/C/"+to_string(BIDDERS)+"/";
    std::filesystem::create_directories(directory);
    std::string directoryr = "N/R/"+to_string(BIDDERS)+"/";
    std::filesystem::create_directories(directoryr);
    std::string directoryw = "N/W/"+to_string(BIDDERS)+"/";
    std::filesystem::create_directories(directoryw);
    string filenamec=directory+"output_"+to_string(PRICES)+"_"+to_string(my_index)+".txt";
    string filenamer=directoryr+"output_"+to_string(PRICES)+"_"+to_string(my_index)+".txt";
    string filenamew=directoryw+"output_"+to_string(PRICES)+"_"+to_string(my_index)+".txt";
    std::ofstream outfile(filenamec, std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return;
    }
    outfile <<std::fixed << std::setprecision(8) << time_select << ",";
    outfile.close();
    std::ofstream outfiler(filenamer, std::ios::app);
    if (!outfiler.is_open()) {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return;
    }
    outfiler <<std::fixed << std::setprecision(8) <<time_to_read << ",";
    outfiler.close();
     std::ofstream outfilew(filenamew, std::ios::app);
    if (!outfilew.is_open()) {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return;
    }
    outfilew <<std::fixed << std::setprecision(8)<< time_to_write << ",";    
    outfilew.close();
    return;
}
int32_t main(int32_t argv,char *argc[]) {  
    int my_bid;int my_index=1;
    my_index=stoi(argc[1]);
    BIDDERS=stoi(argc[2]);
    PRICES=stoi(argc[3]);
    vector<vector<int>> received_values(BIDDERS,vector<int>(PRICES,0));
    vector<vector<int>> sent_values(BIDDERS,vector<int>(PRICES,0));
    vector<int> Y(PRICES,0);
    uniform_int_distribution<> sel(0,PRICES-1);
    my_bid=sel(gen);
    cout<<my_index<<" "<<my_bid<<endl;
    double time_select_start = (double) clock();            /* get initial time */
    time_select_start = time_select_start / CLOCKS_PER_SEC;
    auto transfer_start = std::chrono::steady_clock::now();
    

    for(int i=0;i<PRICES;i++){
        Y[i]=distr(gen);
        int sum=0;
        for(int j=0;j<BIDDERS-1;j++){
            sent_values[j][i]=distr(gen);
            sum+=sent_values[j][i];
            sum%=MOD;
        }
        if(i<=my_bid){
            sent_values[BIDDERS-1][i]=(MOD+Y[i]-sum)%MOD;
        }
        else sent_values[BIDDERS-1][i]=(MOD-sum);
    }
    auto transfer_end = std::chrono::steady_clock::now();
    double time_select = std::chrono::duration<double>(transfer_end - transfer_start).count();
    received_values[my_index]=sent_values[my_index];
    int my_port = 12345+my_index;
    std::thread server_thread(start_server, my_port, std::ref(received_values));
    std::this_thread::sleep_for(std::chrono::seconds((90)));
    std::vector<std::thread> bidder_threads;

    for (int i = 0; i < BIDDERS; i++) {
        if (i == my_index) continue;
        bidder_threads.emplace_back(
            connect_to_bidder, 12345 + i, std::ref(sent_values[i]), my_port
        );
    }
    for (auto& thread : bidder_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    server_thread.join();
    vector<int> publish(PRICES,0);
    for(int i=0;i<BIDDERS;i++){
        for(int j=0;j<PRICES;j++){
            publish[j]+=received_values[i][j];
            publish[j]%=MOD;
        }
    }
    save_to_file(time_select,my_index);
    try{
        int port=1234;
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        boost::asio::write(socket, boost::asio::buffer(publish.data(), PRICES*sizeof(int)));
        socket.close();
    }catch (std::exception& e) {
        std::cerr << "Error connecting to bidder at port " << 1234 << ": " << e.what() << "\n";
    }

    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), my_port));
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    struct winner result;

    boost::asio::read(socket, boost::asio::buffer(&result,(sizeof(winner))));
    if(result.price>=0&&result.price<PRICES){
        if(Y[result.price]==result.value){
            cout<<"I am the winner at price index "<<result.price<<endl;
            return 0;
        }
    }
    cout<<"I did not win\n";
    return 0;
}
