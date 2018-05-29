/* Copyright 2013-2018 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "IpcClient.h"

IpcClient::IpcClient(std::string socketPath) : IIpcClient(socketPath)
{
    Ipc::Output::setLogLevel(-1);
}

IpcClient::~IpcClient()
{
}

void IpcClient::onConnect()
{
    if(_onConnect) _onConnect();
}

// {{{ RPC methods
Ipc::PVariable IpcClient::broadcastEvent(Ipc::PArray& parameters)
{
    if(parameters->size() != 4) return Ipc::Variable::createError(-1, "Wrong parameter count.");

    for(uint32_t i = 0; i < parameters->at(2)->arrayValue->size(); ++i)
    {
        if(_broadcastEvent) _broadcastEvent((uint64_t)parameters->at(0)->integerValue64, parameters->at(1)->integerValue, parameters->at(2)->arrayValue->at(i)->stringValue, parameters->at(3)->arrayValue->at(i));
    }

    return std::make_shared<Ipc::Variable>();
}
// }}}
