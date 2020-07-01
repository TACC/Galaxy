// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#pragma once

/*! \file UnitTest.h 
 * \brief The core class for unit testing within Galaxy
 * \ingroup unittest
 */

/*! \defgroup unittest UnitTest
 * \brief all classes for Galaxy's unit testing framework
 */

#include <iostream>
#include <string>

namespace gxy
{
	//! class for unit test messaging and (perhaps) control
	/*! 
	 * This class provides unit testing messaging services for Galaxy.
	 * Unit test controls might be added in the future.
	 *
	 * \ingroup unittest
	 */
	class UnitTest {
	public:
		UnitTest() //!< default constructor
			: warnings_(0), errors_(0), name_(std::string(""))
		{}
		UnitTest(std::string testname) //!< named constructor
			: warnings_(0), errors_(0), name_(testname)
		{}
		~UnitTest() //!< default destructor
		{}

		//! write a test start message to the specified output stream
		/*! 
		 * \param s output stream target for message (default stdout)
		 */
		void start(std::ostream& s = std::cout)
		{ s << "   GXY:  starting unit test " << name_ << "\n"; }

		//! write a test update message to the specified output stream
		/*! 
		 * \param message optional message text
		 * \param doneness optional progress percentage for unit test (range [0,1])
		 * \param s output stream target for message (default stdout)
		 */
		void update(std::string message = std::string(""), float doneness = 0.f,  std::ostream& s = std::cout)
		{ 
			s << "   GXY:      "; 
			s << "<" << name_ << "> "; 
			s << message;
			if (doneness > 0.f) { int d=doneness/100; s << " (" << d << "%)"; }
			s << "\n";
		}

		//! write a test finish message to the specified output stream
		/*! The message includes the number of warnings and errors written
		 * \param s output stream target for message (default stdout)
		 */
		void finish(std::ostream& s = std::cout)
		{ s << "   GXY:  finished unit test " << name_ << " ( " << warnings_ << " W / " << errors_ << " E )\n"; }

		//! write a test warning message to the specified output stream
		/*! 
		 * \param message optional message text
		 * \param s output stream target for message (default stdout)
		 */
		void warning(std::string message = std::string(""), std::ostream& s = std::cout)
		{
			warnings_++;
			s << "   GXY: ~~~~ WARNING "; 
			s << "<" << name_ << "> ";   
			s << message;
			s << "\n";
		}

		//! write a test error message to the specified output stream
		/*! 
		 * \param message optional message text
		 * \param s output stream target for message (default stdout)
		 */
		void error(std::string message = std::string(""), std::ostream& s = std::cout)
		{
			errors_++;
			s << "   GXY: **** ERROR "; 
			s << "<" << name_ << "> ";  
			s << message;
			s << "\n"; 
		}

		//! reset the warning and error counts for this UnitTest
		void reset()
		{ warnings_ = 0; errors_ = 0; }

		//! returns the number of warnings written
		int warnings()
		{ return warnings_; }

		//! returns the number of errors written
		int errors()
		{ return errors_; }

		//! returns the name of this UnitTest
		std::string name()
		{ return name_; }

	protected:
		int warnings_;
		int errors_;
		std::string name_;
	};
}