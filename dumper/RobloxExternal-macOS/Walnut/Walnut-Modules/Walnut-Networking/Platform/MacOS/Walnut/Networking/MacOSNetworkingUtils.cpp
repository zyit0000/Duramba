#include "Walnut/Networking/NetworkingUtils.h"

#include <iostream>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <print>

namespace Walnut::Utils {

	std::string ResolveDomainName(std::string_view name)
	{
		addrinfo hints{};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		addrinfo* res = nullptr;
		if (int status = getaddrinfo(name.data(), nullptr, &hints, &res); status != 0)
			return {};

		std::string out;
		char buffer[INET6_ADDRSTRLEN];

		for (auto* p = res; p; p = p->ai_next)
		{
			void* addr = nullptr;

			if (p->ai_family == AF_INET)
				addr = &reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr;
			else if (p->ai_family == AF_INET6)
				addr = &reinterpret_cast<sockaddr_in6*>(p->ai_addr)->sin6_addr;
			else
				continue;

			if (inet_ntop(p->ai_family, addr, buffer, sizeof(buffer)))
			{
				out = buffer;
				break;
			}
		}

		freeaddrinfo(res);
		return out;
	}

}