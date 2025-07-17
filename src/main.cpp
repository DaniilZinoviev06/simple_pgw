// здесь больше для себя пометки-напоминания
// TEID - идентификатор в протоколе GTP-U для маршрут. трафика
// PDN(содержит выд. для юзера ip, IMSI абонента, cписок EPS биреров)
// control plane создает или удал. PDN, авктивирует и диактивирует биреры
// data plane передает пользовательский трафик

#include <boost/asio/ip/address.hpp>
#include <iostream>
#include "pdn_connection.h"
#include "control_plane.h"
#include "data_plane.h"

using std::cerr;
using std::endl;
using std::cout;

// небольшой пример работы pgw

class data_plane_heir : public data_plane {
public:
    using data_plane::data_plane;

protected:
    void forward_packet_to_sgw(boost::asio::ip::address_v4 sgw_addr,
        uint32_t sgw_dp_teid, Packet &&packet) override {

        printf("\n%3s\t\t%3s\t\t%3s\n", "TEID", "ip", "Размер пакета");
        printf("%s\t\t%s\t\t%s\n",
            std::to_string(sgw_dp_teid).c_str(), sgw_addr.to_string().c_str(), std::to_string(packet.size()).c_str());
    }

    void forward_packet_to_apn(boost::asio::ip::address_v4 apn_gateway,
                              Packet &&packet) override {

        cout << apn_gateway.to_string().c_str() << std::to_string(packet.size()) << endl;
    }
};

int main() {
    // здесь просто посмотрел на уже реализованный компоненты, немного опробовал
    // auto apn_gateway_ip = boost::asio::ip::make_address_v4("10.0.2.1");
    // auto ue_ip = boost::asio::ip::make_address_v4("94.17.31.62");
    // uint32_t control_teid = 152731;

    // std::shared_ptr<pdn_connection> connection_pdn = pdn_connection::create(control_teid, apn_gateway_ip, ue_ip);
    // connection_pdn->set_sgw_addr(boost::asio::ip::make_address_v4("10.0.2.12"));

    // std::cout << "\nUE IP: " << connection_pdn->get_ue_ip_addr() << std::endl;
    // std::cout << "APN GW: " << connection_pdn->get_apn_gw() << std::endl;

    control_plane cp;
    data_plane_heir dp(cp);
    data_plane::Packet packet1 = {1, 2, 3, 4};
    data_plane::Packet packet2 = {5, 4, 3, 2, 1};
    cp.add_apn("ims", boost::asio::ip::make_address_v4("10.0.2.1"));
    std::shared_ptr<pdn_connection> pdn = cp.create_pdn_connection("ims",
         boost::asio::ip::make_address_v4("94.17.31.62"), 7865);
    std::shared_ptr<bearer> bearer = cp.create_bearer(pdn, 54321);

    bearer->set_rate_limits(0.00008, 0.0008);

    std::thread([&cp]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cp.reset_all_limits();
        }
    }).detach();

    if (!pdn) {
        cerr << "не удалось создать pdn подключение" << endl;
        return -1;
    }

    for (int i = 0; i < 5; ++i) {
        data_plane::Packet packet = {1, 2, 3, 4, 5, 6, 7, 8};
        dp.handle_uplink(bearer->get_sgw_dp_teid(), std::move(packet));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (int i = 0; i < 5; ++i) {
        data_plane::Packet packet = {5, 4, 3, 2, 1};
        dp.handle_downlink(pdn->get_ue_ip_addr(), std::move(packet));
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    //dp.handle_uplink(bearer->get_sgw_dp_teid(), std::move(packet1));

    //dp.handle_downlink(pdn->get_ue_ip_addr(), std::move(packet2));

    //cp.delete_pdn_connection(7865);

    return 0;
}
