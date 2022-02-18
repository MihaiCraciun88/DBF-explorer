#pragma once

#include <exception>

struct Exception : public std::exception {
	const char* pError;
	Exception(const char* error) {
		pError = error;
	}
	const char* what() const throw () {
		return pError;
	}
};