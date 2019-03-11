// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

/*! \file ClientServer.h 
 * \brief configures and establishes client-server socket connectivity for Galaxy communication
 * \ingroup framework
 */

namespace gxy
{

//! configures and establishes client-server socket connectivity for Galaxy communication
/*! \ingroup framework
 */	
class ClientServer 
{
public: 	
	ClientServer(); //!< default constructor

	//! configure and launch the client socket communicator
	/*! \param server_host the server hostname
	 *  \param port on which to establish the socket connection
	 *  \warning server must be setup prior to calling this method
	 *  \sa setup_server
	 */
	void setup_client(char *server_host, int port = 1900);
	//! configure and launch the server socket communicator
	/*! \param port on which to establish the socket connection
	 *  \warning this method must be called prior to client setup
	 *  \sa setup_client
	 */
	void setup_server(int port = 1900);
	//! get the socket number for this client-server connection
	/*! \returns the socket number
	 */
	int  get_socket() { return skt; }

private:
	int skt;
};

} // namespace gxy
