#include "Debug.h"

using namespace std;

Logger::Logger()
{
    
}

void Logger::Initialize()
{
    using namespace spdlog;
    sinks.push_back(make_shared<sinks::wincolor_stdout_sink_mt>("console"));
    sinks.push_back(make_shared<sinks::simple_file_sink_mt>("Seaurchin.log"));

    loggerMain = make_shared<logger>("main", begin(sinks), end(sinks));
    register_logger(loggerMain);

    AllocConsole();
}

void Logger::Terminate()
{
    spdlog::drop_all();
    FreeConsole();
}