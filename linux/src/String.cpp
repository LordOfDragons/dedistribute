/*
 * MIT License
 *
 * Copyright (C) 2024, DragonDreams GmbH (info@dragondreams.ch)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdio.h>
#include "String.h"
#include "Exception.h"

String::String(){
	pString = new char[1];
	pString[0] = 0;
}

String::String(const String &string){
	pString = new char[string.Length() + 1];
	strcpy(pString, string.pString);
}

String::String(const char *string){
	pString = new char[strlen(string) + 1];
	strcpy(pString, string);
}

String::~String(){
	if(pString){
		delete [] pString;
	}
}

const char *String::Pointer() const{
	return pString;
}

int String::Length() const{
	return strlen(pString);
}

int String::Find(char delimiter) const{
	const char *s = strchr(pString, delimiter);
	if(s){
		return (int)(s - pString);
	}
	return -1;
}

String String::SubString(int from) const{
	return SubString(from, Length());
}

String String::SubString(int from, int to) const{
	const int len = Length();
	if(from < 0){
		from += len;
	}
	if(to < 0){
		to += len;
	}
	
	if(from < 0 || from >= len || to < 0 || to > len){
		throw Exception("Index out of bounds");
	}
	
	const int nlen = to - from;
	String s;
	delete [] s.pString;
	s.pString = new char[nlen + 1];
	strncpy(s.pString, pString + from, nlen);
	s.pString[nlen] = 0;
	return s;
}

String::operator const char*() const{
	return pString;
}

char String::operator[](int index) const{
	if(index < 0){
		index += Length();
	}
	if(index < 0 || index >= Length()){
		throw Exception("Index out of range");
	}
	return pString[index];
}

String String::operator+(const String &string) const{
	const int len = Length();
	String nstr;
	delete [] nstr.pString;
	nstr.pString = new char[len + string.Length() + 1];
	strcpy(nstr.pString, pString);
	strcpy(nstr.pString + len, string.pString);
	return nstr;
}

String String::operator+(char character) const{
	const int len = Length();
	String nstr;
	delete [] nstr.pString;
	nstr.pString = new char[len + 2];
	strcpy(nstr.pString, pString);
	nstr.pString[len] = character;
	nstr.pString[len + 1] = 0;
	return nstr;
}

String String::operator=(const String &string){
	if(pString){
		delete [] pString;
	}
	pString = new char[string.Length() + 1];
	strcpy(pString, string.pString);
	return *this;
}

String String::operator+=(const String &string){
	return *this = *this + string;
}

String String::operator+=(char character){
	return *this = *this + character;
}

bool String::operator==(const String &string) const{
	return strcmp(pString, string.pString) == 0;
}

bool String::operator==(const char *string) const{
	return strcmp(pString, string) == 0;
}

bool String::operator!=(const String &string) const{
	return strcmp(pString, string.pString) != 0;
}

bool String::operator!=(const char *string) const{
	return strcmp(pString, string) != 0;
}
