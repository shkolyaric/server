#include<iostream>
//библиотека asio для работы с сетью
#include<boost/asio.hpp>
//библиотека с умными указателями
#include<memory>
// библиотеки для работы с json
#include<boost/property_tree/json_parser.hpp>
#include<boost/property_tree/ptree.hpp>

//библиотеки для логирования
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <fstream>
#include <boost/log/utility/setup/common_attributes.hpp>
//самописная библиотека для вычисление мат выражения
#include "evaluate.h"
//метод который возвращает поле из JSON
std::string getfield(char json[],size_t len, std::string field)
{
    std::stringstream jsonEnc;
    jsonEnc.write(json,len);
    boost::property_tree::ptree root;
    boost::property_tree::read_json(jsonEnc,root);
    return root.get<std::string>(field);
}

/*метод создаёт json в виде строки вида:
   {
   "resp":"ответ на запрос"
   }
*/
std::string genjson(std::string resp)
{
    boost::property_tree::ptree root;
    root.put("resp",resp);
    std::ostringstream oss;
    boost::property_tree::write_json(oss, root,false);
    return oss.str();
}
//инициализируем logger
void init_logger()
{
    namespace logging = boost::log;
    namespace sinks = logging::sinks;
    namespace expr = logging::expressions;
    namespace attrs = logging::attributes;
    using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;
    auto sink = boost::make_shared<text_sink>();
    //подключаем выходной файловый поток
    boost::shared_ptr<std::ostream> stream(boost::shared_ptr< std::ostream >(new std::ofstream("run.log")));
    sink->locked_backend()->add_stream(stream);

    // ставим выходной формат
    sink->set_formatter(
        expr::stream
        << '['
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
        << "] "
        << expr::smessage
    );
    logging::add_common_attributes();

    logging::core::get()->add_sink(sink);
}

class connection 
   : public std::enable_shared_from_this<connection>
{
private:
    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 2048 };
    std::string data;
    char data_[max_length];
public:
    connection(boost::asio::ip::tcp::socket soc) : socket_(std::move(soc))
    {
    }
    void start()
    {
        BOOST_LOG_TRIVIAL(info) << "connected to "<<socket_.remote_endpoint().address()<<'\n';
        do_read();
    }
private:
    void do_read()
    {
        BOOST_LOG_TRIVIAL(info) << socket_.remote_endpoint().address() << "try to read \n";
        socket_.async_read_some(boost::asio::buffer(data_,max_length),
            [self=shared_from_this()](std::error_code ec, std::size_t len)
            {
                if (!ec)
                {
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "read end\n";

                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "received from the client:";
                    BOOST_LOG_TRIVIAL(info).write(self->data_, len);
                    BOOST_LOG_TRIVIAL(info) << '\n';
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "evaluate express\n";
                    self->data = genjson(eval(getfield(self->data_, len, "req")));
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "result of evaluation " << self->data << '\n';
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << " try to send\n";
                    self->do_write(len);
                }
                else if(ec.message()!="End of file")
                {
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "read failed\n";
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "ERROR:" << ec.message() << '\n';
                }
            });
    }
    void do_write(size_t len)
    {
        
        boost::asio::async_write(socket_, boost::asio::buffer(data),
            [self = shared_from_this()](std::error_code ec,size_t)
            {
                if (!ec)
                {
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << " eval send already\n";
                    self->socket_.close();
                }
                else
                {
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "write failed\n";
                    BOOST_LOG_TRIVIAL(info) << self->socket_.remote_endpoint().address() << "ERROR:" << ec.message() << '\n';
                }
            });
        
    }
};
void serve(boost::asio::ip::tcp::acceptor& acceptor_)
{
    acceptor_.async_accept(
        [& acceptor_](std::error_code ec, boost::asio::ip::tcp::socket soc)
        {
            serve(acceptor_);
            if (!ec)
            {
                std::make_shared<connection>(std::move(soc))->start();
            }
            
        }
    );
}
#include <fstream>
int main()
{
    init_logger();
    try
    {
        boost::asio::io_context context;
        boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 1337));
        serve(acceptor);
        context.run();
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL(info) << "failed to start\n";
        BOOST_LOG_TRIVIAL(info) << "exception:" << e.what() << '\n';
    }
    system("pause");
}
