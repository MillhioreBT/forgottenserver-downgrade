#ifndef FS_TESTS_UNIT_SAVEDCONDITIONS_H
#define FS_TESTS_UNIT_SAVEDCONDITIONS_H

#include "condition.h"
#include "fileloader.h"

inline std::string saveConditions(std::initializer_list<Condition*> conditions)
{
	PropWriteStream stream;
	for (Condition* condition : conditions) {
		condition->serialize(stream);
		stream.write<uint8_t>(CONDITIONATTR_END);
	}
	return std::string{stream.getStream()};
}

inline std::vector<Condition_ptr> loadConditions(const std::string& saved)
{
	PropStream stream;
	stream.init(saved.data(), saved.size());

	std::vector<Condition_ptr> loaded;
	for (auto condition = Condition::createCondition(stream); condition;
	     condition = Condition::createCondition(stream)) {
		if (condition->unserialize(stream)) {
			loaded.push_back(std::move(condition));
		}
	}
	return loaded;
}

#endif
