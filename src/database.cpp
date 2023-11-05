// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "database.h"

#include "configmanager.h"

#include <mysql/errmsg.h>

extern ConfigManager g_config;

Database::~Database()
{
	if (handle != nullptr) {
		mysql_close(handle);
	}
}

bool Database::connect()
{
	// connection handle initialization
	handle = mysql_init(nullptr);
	if (!handle) {
		std::cout << std::endl << "Failed to initialize MySQL connection handle." << std::endl;
		return false;
	}

	// automatic reconnect
	bool reconnect = true;
	mysql_options(handle, MYSQL_OPT_RECONNECT, &reconnect);

	// connects to database
	if (!mysql_real_connect(handle, g_config[ConfigKeysString::MYSQL_HOST].data(),
	                        g_config[ConfigKeysString::MYSQL_USER].data(),
	                        g_config[ConfigKeysString::MYSQL_PASS].data(), g_config[ConfigKeysString::MYSQL_DB].data(),
	                        g_config[ConfigKeysInteger::SQL_PORT], g_config[ConfigKeysString::MYSQL_SOCK].data(), 0)) {
		std::cout << std::endl << "MySQL Error Message: " << mysql_error(handle) << std::endl;
		return false;
	}

	DBResult_ptr result = storeQuery("SHOW VARIABLES LIKE 'max_allowed_packet'");
	if (result) {
		maxPacketSize = result->getNumber<uint64_t>("Value");
	}
	return true;
}

bool Database::beginTransaction()
{
	if (!executeQuery("BEGIN")) {
		return false;
	}

	databaseLock.lock();
	return true;
}

bool Database::rollback()
{
	if (mysql_rollback(handle) != 0) {
		std::cout << "[Error - mysql_rollback] Message: " << mysql_error(handle) << std::endl;
		databaseLock.unlock();
		return false;
	}

	databaseLock.unlock();
	return true;
}

bool Database::commit()
{
	if (mysql_commit(handle) != 0) {
		std::cout << "[Error - mysql_commit] Message: " << mysql_error(handle) << std::endl;
		databaseLock.unlock();
		return false;
	}

	databaseLock.unlock();
	return true;
}

bool Database::executeQuery(std::string_view query)
{
	bool success = true;

	// executes the query
	databaseLock.lock();

	while (mysql_real_query(handle, query.data(), query.length()) != 0) {
		std::cout << "[Error - mysql_real_query] Query: " << query.substr(0, 256) << std::endl
		          << "Message: " << mysql_error(handle) << std::endl;
		auto error = mysql_errno(handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR &&
		    error != 1053 /*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			success = false;
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	MYSQL_RES* m_res = mysql_store_result(handle);
	databaseLock.unlock();

	if (m_res) {
		mysql_free_result(m_res);
	}

	return success;
}

DBResult_ptr Database::storeQuery(std::string_view query)
{
	databaseLock.lock();

retry:
	while (mysql_real_query(handle, query.data(), query.length()) != 0) {
		std::cout << "[Error - mysql_real_query] Query: " << query << std::endl
		          << "Message: " << mysql_error(handle) << std::endl;
		auto error = mysql_errno(handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR &&
		    error != 1053 /*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// we should call that every time as someone would call executeQuery('SELECT...')
	// as it is described in MySQL manual: "it doesn't hurt" :P
	MYSQL_RES* res = mysql_store_result(handle);
	if (res == nullptr) {
		std::cout << "[Error - mysql_store_result] Query: " << query << std::endl
		          << "Message: " << mysql_error(handle) << std::endl;
		auto error = mysql_errno(handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR &&
		    error != 1053 /*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			databaseLock.unlock();
			return nullptr;
		}
		goto retry;
	}
	databaseLock.unlock();

	// retrieving results of query
	DBResult_ptr result = std::make_shared<DBResult>(res);
	if (!result->hasNext()) {
		return nullptr;
	}
	return result;
}

std::string Database::escapeString(std::string_view s) const { return escapeBlob(s.data(), s.length()); }

std::string Database::escapeBlob(const char* s, uint32_t length) const
{
	// the worst case is 2n + 1
	size_t maxLength = (length * 2) + 1;

	std::string escaped;
	escaped.reserve(maxLength + 2);
	escaped.push_back('\'');

	if (length != 0) {
		char* output = new char[maxLength];
		mysql_real_escape_string(handle, output, s, length);
		escaped.append(output);
		delete[] output;
	}

	escaped.push_back('\'');
	return escaped;
}

DBResult::DBResult(MYSQL_RES* res)
{
	handle = res;

	size_t i = 0;

	MYSQL_FIELD* field = mysql_fetch_field(handle);
	while (field) {
		listNames[field->name] = i++;
		field = mysql_fetch_field(handle);
	}

	row = mysql_fetch_row(handle);
}

DBResult::~DBResult() { mysql_free_result(handle); }

std::string_view DBResult::getString(const std::string& s) const
{
	auto it = listNames.find(s);
	if (it == listNames.end()) {
		std::cout << "[Error - DBResult::getString] Column '" << s << "' does not exist in result set." << std::endl;
		return "";
	}

	if (row[it->second] == nullptr) {
		return "";
	}

	return row[it->second];
}

std::string_view DBResult::getStream(const std::string& s, unsigned long& size) const
{
	auto it = listNames.find(s);
	if (it == listNames.end()) {
		std::cout << "[Error - DBResult::getStream] Column '" << s << "' doesn't exist in the result set" << std::endl;
		size = 0;
		return "";
	}

	if (row[it->second] == nullptr) {
		size = 0;
		return "";
	}

	size = mysql_fetch_lengths(handle)[it->second];
	return row[it->second];
}

bool DBResult::hasNext() const { return row != nullptr; }

bool DBResult::next()
{
	row = mysql_fetch_row(handle);
	return row != nullptr;
}

DBInsert::DBInsert(std::string_view query) : query{query} { this->length = this->query.length(); }

bool DBInsert::addRow(std::string_view row)
{
	// adds new row to buffer
	const size_t rowLength = row.length();
	length += rowLength;
	if (length > Database::getInstance().getMaxPacketSize() && !execute()) {
		return false;
	}

	if (values.empty()) {
		values.reserve(rowLength + 2);
		values.push_back('(');
		values.append(row);
		values.push_back(')');
	} else {
		values.reserve(values.length() + rowLength + 3);
		values.push_back(',');
		values.push_back('(');
		values.append(row);
		values.push_back(')');
	}
	return true;
}

bool DBInsert::addRow(std::ostringstream& row)
{
	bool ret = addRow(row.str());
	row.str(std::string());
	return ret;
}

bool DBInsert::execute()
{
	if (values.empty()) {
		return true;
	}

	// executes buffer
	bool res = Database::getInstance().executeQuery(query + values);
	values.clear();
	length = query.length();
	return res;
}
