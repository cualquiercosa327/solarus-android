/*
 * Copyright (C) 2009-2011 Christopho, Solarus - http://www.solarus-engine.org
 * 
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SOLARUS_DEBUG_H
#define SOLARUS_DEBUG_H

#include "Common.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#ifdef ANDROID
#include <android/log.h>
#endif
#ifndef NDEBUG
#define SOLARUS_ASSERT(condition, message) Debug::check_assertion(condition, message)
#else
#define SOLARUS_ASSERT(condition, message)
#endif

/**
 * @brief Provides functionalities for printing debug messages or making
 * runtime verifications, especially when the code is compiled in debugging
 * mode.
 */
class Debug {

  private:

    Debug();    // don't instantiate this class

  public:

    static void print(const std::string& message, std::ostream& os = std::cout);
    static void check_assertion(bool assertion, const std::string& error_message = "");
    static void die(const std::string& error_message = "");
};


/**
 * @brief Prints a message if the program is in debug mode.
 *
 * This function does nothing if NDEBUG is defined.
 *
 * @param message the message to print.
 * @param os the output stream to write (default is std::cout)
 */
inline void Debug::print(const std::string& message, std::ostream& os) {
#ifdef ANDROID
	__android_log_print(ANDROID_LOG_INFO, "Solarus engine", message.c_str());
#endif
#ifndef NDEBUG
  os << message << std::endl;
#endif
}

/**
 * @brief Throws an exception if the specified assertion fails.
 *
 * If the assertion fails, an std::logic_error with the specified error message is thrown.
 * The error message is saved in error.txt.
 * This function should be used to detect fatal errors only, that is,
 * errors in your code or in the quest (the data files) that require to stop the program.
 *
 * @param assertion the boolean condition to check
 * @param error_message the error message to attach to the exception when the assertion fails
 */
inline void Debug::check_assertion(bool assertion, const std::string& error_message) {

  if (!assertion) {
#ifdef ANDROID
	__android_log_print(ANDROID_LOG_INFO, "Solarus engine", error_message.c_str());
#endif
    die(error_message);
  }
}

/**
 * @brief Throws an exception to stop the program.
 *
 * This function is equivalent to assert(false, error_message).
 * The error message is saved in error.txt.
 *
 * @param error_message the error message to attach to the exception
 */
inline void Debug::die(const std::string& error_message) {

  std::ofstream out("error.txt");
  out << error_message << std::endl << std::flush;
  throw std::logic_error(error_message);
}

#endif

