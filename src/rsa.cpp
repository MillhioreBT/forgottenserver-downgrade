// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "rsa.h"

#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>
#include <fstream>
#include <sstream>

static CryptoPP::AutoSeededRandomPool prng;

void RSA::decrypt(char* msg) const
{
	try {
		CryptoPP::Integer m{reinterpret_cast<uint8_t*>(msg), 128};
		auto c = pk.CalculateInverse(prng, m);
		c.Encode(reinterpret_cast<uint8_t*>(msg), 128);
	} catch (const CryptoPP::Exception& e) {
		std::cout << e.what() << '\n';
	}
}

static const std::string header = "-----BEGIN RSA PRIVATE KEY-----";
static const std::string footer = "-----END RSA PRIVATE KEY-----";

void RSA::loadPEM(const std::string& filename)
{
	std::ifstream file{filename};

	if (!file.is_open()) {
		throw std::runtime_error("Missing file " + filename + ".");
	}

	std::ostringstream oss;
	for (std::string line; std::getline(file, line); oss << line)
		;
	std::string key = oss.str();

	if (key.substr(0, header.size()) != header) {
		throw std::runtime_error("Missing RSA private key header.");
	}

	if (key.substr(key.size() - footer.size(), footer.size()) != footer) {
		throw std::runtime_error("Missing RSA private key footer.");
	}

	key = key.substr(header.size(), key.size() - footer.size());

	CryptoPP::ByteQueue queue;
	CryptoPP::Base64Decoder decoder;
	decoder.Attach(new CryptoPP::Redirector(queue));
	decoder.Put(reinterpret_cast<const uint8_t*>(key.c_str()), key.size());
	decoder.MessageEnd();

	try {
		pk.BERDecodePrivateKey(queue, false, queue.MaxRetrievable());

		if (!pk.Validate(prng, 3)) {
			throw std::runtime_error("RSA private key is not valid.");
		}
	} catch (const CryptoPP::Exception& e) {
		std::cout << e.what() << '\n';
	}
}
