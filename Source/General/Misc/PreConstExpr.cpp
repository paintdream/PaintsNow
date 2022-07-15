#include "PreConstExpr.h"
using namespace PaintsNow;

PreConstExpr::EvalResult PreConstExpr::Evaluate(const char* begin, const char* end) const {
	// simply we only resolve names
	bool inverse = false;
	if (*begin == '!') {
		inverse = true;
		begin++;
	}

	size_t length = end - begin;
	for (size_t i = 0; i < variables.size(); i++) {
		const IAsset::Material::Variable& variable = variables[i];
		if (variable.key.GetSize() == length && memcmp(variable.key.GetData(), begin, length) == 0) {
			bool result = variable.type == IAsset::TYPE_CONST ? variable.value[0] != 0 : false;
			return result != inverse ? EVAL_TRUE : EVAL_FALSE;
		}
	}

	// not constant ?
	return EVAL_DYNAMIC;
}

String PreConstExpr::operator () (const String& text) const {
	// find 'if constexpr'
	String target;
	size_t lastPos = 0;
	size_t length = text.length();
	size_t i = 0;
	bool continuePass = false;
	while (true) {
		size_t pos = text.find("if (", i);
		if (pos == String::npos) {
			target.append(text.begin() + lastPos, text.end());
			return continuePass ? (*this)(target) : target;
		}

		target.append(text.begin() + lastPos, text.begin() + pos);

		// search condition
		bool findCondition = false;
		bool finish = false;
		EvalResult evalCondition = EVAL_DYNAMIC;
		// bool findIfPart = false;
		// bool findElsePart = false;

		uint32_t parenthesesCount = 0;
		uint32_t bracesCount = 0;
		uint32_t conditionStart = 0;
		uint32_t partStart = 0;

		for (i = pos; i < length && !finish; i++) {
			char ch = text[i];
			switch (ch) {
				case '(':
					if (parenthesesCount++ == 0) {
						if (!findCondition) {
							conditionStart = verify_cast<uint32_t>(i + 1);
						}
					}
					break;
				case ')':
					if (--parenthesesCount == 0) {
						if (!findCondition) {
							evalCondition = Evaluate(text.data() + conditionStart, text.data() + i);
							findCondition = true;
						}
					}
					break;
				case '{':
					if (bracesCount++ == 0) {
						partStart = verify_cast<uint32_t>(i + 1);
					}

					break;
				case '}':
					if (--bracesCount == 0) {
						assert(findCondition);

						if (true || evalCondition == EVAL_DYNAMIC) {
							target.append(text.begin() + pos, text.begin() + i + 1);
						} else {
							if (evalCondition == EVAL_TRUE) {
								target.append(text.begin() + partStart, text.begin() + i);
							}
							continuePass = true;
						}

						lastPos = i + 1;
						finish = true; // break out
					}
					break;
			}
		}
	}
}
