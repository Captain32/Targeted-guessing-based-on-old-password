#ifndef ALPHANUMPAIR_H
#define ALPHANUMPAIR_H

#define MAX_WORD_LENGTH 40

class alphaNumPair
{
public:
	char ch;
	bool origflag = false;
	bool delflag=false;
	bool swapflag = false;
	int num;
	int nowpos, aimpos;
	char segment[MAX_WORD_LENGTH];
	int charpos[MAX_WORD_LENGTH];
	alphaNumPair();
	~alphaNumPair();
	const bool operator==(const alphaNumPair & x);
	const bool operator<(const alphaNumPair & x);
	const bool operator>(const alphaNumPair & x);
};

alphaNumPair::alphaNumPair()
{
	ch = 0;
	num = 0;
}

alphaNumPair::~alphaNumPair()
{
}

const bool alphaNumPair::operator==(const alphaNumPair & x)
{
	return ch == x.ch && num == x.num;
}

const bool alphaNumPair::operator<(const alphaNumPair &x)
{
	if (ch < x.ch) return true;
	if (ch > x.ch) return false;
	return num < x.num;
}

const bool alphaNumPair::operator>(const alphaNumPair &x)
{
	if (ch > x.ch) return true;
	if (ch < x.ch) return false;
	return num > x.num;
}
#endif