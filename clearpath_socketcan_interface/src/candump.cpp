// Copyright (c) 2016-2019, Mathias Lüdtke, AutonomouStuff
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <memory>
#include <unordered_set>
#include <string>

#include <boost/exception/diagnostic_information.hpp>
#include <pluginlib/class_loader.hpp>

#include "clearpath_socketcan_interface/socketcan.hpp"

void print_error(const can::State & s);

void print_frame(const can::Frame & f)
{
  if (f.is_error) {
    std::cout << "E " << std::hex << f.id << std::dec;
  } else if (f.is_extended) {
    std::cout << "e " << std::hex << f.id << std::dec;
  } else {
    std::cout << "s " << std::hex << f.id << std::dec;
  }

  std::cout << "\t";

  if (f.is_rtr) {
    std::cout << "r";
  } else {
    std::cout << static_cast<int>(f.dlc) << std::hex;

    for (int i = 0; i < f.dlc; ++i) {
      std::cout << std::hex << " " << static_cast<int>(f.data[i]);
    }
  }

  std::cout << std::dec << std::endl;
}

std::shared_ptr<class_loader::ClassLoader> g_loader;
can::DriverInterfaceSharedPtr g_driver;

void print_error(const can::State & s)
{
  std::string err;
  g_driver->translateError(s.internal_error, err);
  std::cout << "ERROR: state=" << s.driver_state;
  std::cout << " internal_error=" << s.internal_error;
  std::cout << "('" << err << "') asio: " << s.error_code << std::endl;
}

int main(int argc, char * argv[])
{
  if (argc != 2 && argc != 4) {
    std::cout << "usage: " << argv[0] << " DEVICE [PLUGIN_PATH PLUGIN_NAME]" << std::endl;
    return 1;
  }

  if (argc == 4) {
    try {
      g_loader = std::make_shared<class_loader::ClassLoader>(argv[2]);
      g_driver = g_loader->createUniqueInstance<can::DriverInterface>(argv[3]);
    } catch (std::exception & ex) {
      std::cerr << boost::diagnostic_information(ex) << std::endl;
      return 1;
    }
  } else {
    g_driver = std::make_shared<can::SocketCANInterface>();
  }

  can::FrameListenerConstSharedPtr frame_printer = g_driver->createMsgListener(print_frame);
  can::StateListenerConstSharedPtr error_printer = g_driver->createStateListener(print_error);

  if (!g_driver->init(argv[1], false)) {
    print_error(g_driver->getState());
    return 1;
  }

  g_driver->run();

  g_driver->shutdown();
  g_driver.reset();
  g_loader.reset();

  return 0;
}
