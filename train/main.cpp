#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<fstream>
#include<time.h>
#include<algorithm>
#include<vector>
#include<math.h>
#include<stdio.h>
#include "Trie.h"
#include "BaseStructure.h"
#include "alphaNumPair.h"
using namespace std;

#define FILE_TRAINING "../data/email_sina_csdn_reuse_multi_unique.txt" //训练集文件路径
#define FILE_OUT "../data/email_sina_csdn_reuse_multi_unique_model.txt" //生成的猜测口令输出文件
#define MAXLINE 100000
#define PSWSIM_THRESHOLD 0.5
#define SEGSIM_THRESHOLD 0.4


class Mypsw
{
public:
	char word[MAX_WORD_LENGTH] = { 0 };
	int len;
	BaseStructure BS;
	double prob = 0.0;
	void updateBS();
	void printBS();
	void updateword();
};
Mypsw password[3];
int totalpswnum = 0;
int BSlen[MAX_WORD_LENGTH] = { 0 };
int seglen[MAX_WORD_LENGTH] = { 0 };
int BSlenforadd[MAX_WORD_LENGTH] = { 0 };
int seglenforadd[MAX_WORD_LENGTH] = { 0 };
int delsegposp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
int delseglenp[3][MAX_WORD_LENGTH] = { 0 };
int addsegposp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
int addseglenp[3][MAX_WORD_LENGTH] = { 0 };
int swapsegp[MAX_WORD_LENGTH] = { 0 };
int delcharp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
int addcharp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
int capitalp[5];
int repeatp = 0;

int alpha2index(char c) //L->0,D->1,S->2
{
	if (c == 'L') return 0;
	if (c == 'D') return 1;
	if (c == 'S') return 2;
}

char char2alpha(char c) //a-zA-Z -> L,0-9 -> 1,other -> 2
{
	if (c >= 'A'&&c <= 'Z') return 'L';
	if (c >= 'a'&&c <= 'z') return 'L';
	if (c >= '0'&&c <= '9') return 'D';
	return 'S';
}

void Mypsw::updateBS()
{
	BS.length = 0;
	for (int i = 0; i < len; ) {
		BS.word[BS.length].ch = char2alpha(word[i]);
		int j;
		char tmpseg[MAX_WORD_LENGTH] = { 0 };
		for (j = i; j < len; j++) {
			if (char2alpha(word[i]) != char2alpha(word[j])) break;
			tmpseg[j - i] = word[j];
		}
		tmpseg[j] = 0;
		BS.word[BS.length].num = j - i;
		strcpy(BS.word[BS.length].segment, tmpseg);
		BS.word[BS.length].nowpos = BS.length;
		BS.length += 1;
		i = j;
	}
}

void Mypsw::printBS()
{
	for (int i = 0; i < BS.length; i++)
		printf("%c%d:%s nowpos:%d aimpos:%d\n", BS.word[i].ch, BS.word[i].num, BS.word[i].segment, BS.word[i].nowpos, BS.word[i].aimpos);
	printf("\n");
}

void Mypsw::updateword()
{
	memset(word, 0, sizeof(word));
	len = 0;
	for (int i = 0; i < BS.length; i++) {
		for (int j = 0; j < BS.word[i].num; j++) {
			word[len++] = BS.word[i].segment[j];
		}
	}
}

float edit_dis(char *a, char *b)
{
	int dis[MAX_WORD_LENGTH][MAX_WORD_LENGTH];
	int lena = strlen(a);
	int lenb = strlen(b);
	for (int j = 0; j <= lenb; j++)
		dis[0][j] = j;
	for (int i = 0; i <= lena; i++)
		dis[i][0] = i;
	for (int i = 1; i <= lena; i++) {
		for (int j = 1; j <= lenb; j++) {
			int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
			int mindis = dis[i - 1][j] + 1;
			if (mindis > dis[i][j - 1] + 1)
				mindis = dis[i][j - 1] + 1;
			if (mindis > dis[i - 1][j - 1] + cost)
				mindis = dis[i - 1][j - 1] + cost;
			dis[i][j] = mindis;
		}
	}
	return (float)dis[lena][lenb] / max(lena, lenb);
}

void delseg(Mypsw& apsw, Mypsw bpsw)
{
	int inverse[MAX_WORD_LENGTH];
	memset(inverse, 0xff, sizeof(inverse));
	for (int i = 0; i < apsw.BS.length; i++) {
		apsw.BS.word[i].aimpos = -1;
		float mindis = 1.0;
		for (int j = 0; j < bpsw.BS.length; j++) {
			if (apsw.BS.word[i].ch != bpsw.BS.word[j].ch) continue;
			if (inverse[j] != -1) continue;
			float tmpdis = edit_dis(apsw.BS.word[i].segment, bpsw.BS.word[j].segment);
			if (tmpdis < min(mindis, (float)SEGSIM_THRESHOLD)) {
				if (apsw.BS.word[i].aimpos != -1)
					inverse[apsw.BS.word[i].aimpos] = -1;
				apsw.BS.word[i].aimpos = j;
				inverse[j] = i;
				mindis = tmpdis;
			}
		}
		if (apsw.BS.word[i].aimpos == -1) {
			delsegposp[apsw.BS.length][i] += 1;
			int type = alpha2index(apsw.BS.word[i].ch);
			delseglenp[type][apsw.BS.word[i].num] += 1;
		}
	}
	BaseStructure tmpBS;
	for (int i = 0; i < apsw.BS.length; i++) {
		if (apsw.BS.word[i].aimpos == -1) continue;
		tmpBS.word[tmpBS.length] = apsw.BS.word[i];
		tmpBS.word[tmpBS.length].nowpos = tmpBS.length;
		tmpBS.length++;
	}
	apsw.BS = tmpBS;
	apsw.updateword();
}

void swapseg(Mypsw& apsw)
{
	int sparepos = 0;
	while (sparepos < apsw.BS.length) {
		int minpos = 0x7f7f7f7f, mini;
		for (int i = sparepos; i < apsw.BS.length; i++) {
			if (apsw.BS.word[i].aimpos < minpos) {
				minpos = apsw.BS.word[i].aimpos;
				mini = i;
			}
		}
		swap(apsw.BS.word[sparepos], apsw.BS.word[mini]);
		apsw.BS.word[sparepos].nowpos = sparepos;
		apsw.BS.word[mini].nowpos = mini;
		swapsegp[mini - sparepos] += 1;
		sparepos += 1;
	}
	apsw.updateword();
}

void addseg(Mypsw& apsw, Mypsw bpsw)
{
	bool tmpmark[MAX_WORD_LENGTH];
	memset(tmpmark, false, sizeof(tmpmark));
	BaseStructure tmpBS = bpsw.BS;
	int index = 0, initlen = apsw.BS.length;
	for (int i = 0; i < apsw.BS.length; i++) {
		int apos = apsw.BS.word[i].aimpos;
		tmpBS.word[apos] = apsw.BS.word[i];
		tmpBS.word[apos].origflag = true;
		tmpmark[apos] = true;
	}
	apsw.BS = tmpBS;
	apsw.updateword();
	tmpmark[tmpBS.length] = true;
	for (int i = 0; i <= tmpBS.length; i++) {
		if (!tmpmark[i]) {
			int type = alpha2index(tmpBS.word[i].ch);
			addseglenp[type][tmpBS.word[i].num] += 1;
		}
		else {
			if (i > 0 && !tmpmark[i - 1])
				addsegposp[initlen][index] += 1;
			index += 1;
		}
	}
}

void delchar(alphaNumPair& anp, alphaNumPair bnp)
{
	int dp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
	for (int i = 1; i <= anp.num; i++) {
		for (int j = 1; j <= bnp.num; j++) {
			if (anp.segment[i - 1] == bnp.segment[j - 1])
				dp[i][j] = dp[i - 1][j - 1] + 1;
			else
				dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
		}
	}
	int lcslen = dp[anp.num][bnp.num], slen = bnp.num, x = anp.num, y = bnp.num;
	char tmpstr[MAX_WORD_LENGTH] = { 0 };
	while (x > 0) {
		if (anp.segment[x - 1] == bnp.segment[y - 1]) {
			tmpstr[y - 1] = bnp.segment[y - 1];
			lcslen -= 1;
			x -= 1;
			y -= 1;
		}
		else if (dp[x][y] == dp[x - 1][y]) {
			delcharp[anp.num][x - 1] += 1;
			x -= 1;
		}
		else
			y -= 1;
	}
	memset(anp.segment, 0, sizeof(anp.segment));
	for (int i = 0; i < slen; i++)
		anp.segment[i] = tmpstr[i];
	anp.num = slen;
}

void addchar(alphaNumPair& anp, alphaNumPair bnp)
{
	int initlen = 0, index = 0;
	for (int i = 0; i < bnp.num; i++)
		if (anp.segment[i] != 0) initlen++;
	seglenforadd[initlen] += 1;
	for (int i = 0; i < bnp.num; i++) {
		if (anp.segment[i] == 0) continue;
		if (i > 0 && anp.segment[i - 1] == 0)
			addcharp[initlen][index] += 1;
		index += 1;
	}
	for (int i = 0; i < bnp.num; i++) {
		if (anp.segment[i] == 0) {
			anp.segment[i] = bnp.segment[i];
			if (i == bnp.num - 1)
				addcharp[initlen][initlen] += 1;
			continue;
		}
	}
}

bool Capital(Mypsw apsw, Mypsw bpsw, int type)
{
	if (strcmp(apsw.word, bpsw.word) == 0) return false;
	for (int i = 0; i < apsw.BS.length; i++) { //type:0-首字母大写，1-全大写，2-全小写,3-首字母小写，4-全小写后首字母大写
		if (apsw.BS.word[i].ch != 'L') continue;
		if (type == 0) {
			if (apsw.BS.word[i].segment[0] >= 'a'&&apsw.BS.word[i].segment[0] <= 'z')
				apsw.BS.word[i].segment[0] -= 32;
		}
		else if (type == 1) {
			for (int j = 0; j < apsw.BS.word[i].num; j++) {
				if (apsw.BS.word[i].segment[j] >= 'a'&&apsw.BS.word[i].segment[j] <= 'z')
					apsw.BS.word[i].segment[j] -= 32;
			}
		}
		else if (type == 2) {
			for (int j = 0; j < apsw.BS.word[i].num; j++) {
				if (apsw.BS.word[i].segment[j] >= 'A'&&apsw.BS.word[i].segment[j] <= 'Z')
					apsw.BS.word[i].segment[j] += 32;
			}
		}
		else if (type == 3) {
			if (apsw.BS.word[i].segment[0] >= 'A'&&apsw.BS.word[i].segment[0] <= 'Z')
				apsw.BS.word[i].segment[0] += 32;
		}
		else if (type == 4) {
			for (int j = 0; j < apsw.BS.word[i].num; j++) {
				if (apsw.BS.word[i].segment[j] >= 'A'&&apsw.BS.word[i].segment[j] <= 'Z')
					apsw.BS.word[i].segment[j] += 32;
			}
			apsw.BS.word[i].segment[0] -= 32;
		}
	}
	apsw.updateword();
	if (strcmp(apsw.word, bpsw.word) == 0) return true;
	return false;
}

bool Repeat(Mypsw apsw, Mypsw bpsw)
{
	if (2 * apsw.len != bpsw.len) return false;
	for (int i = 0; i < apsw.len; i++)
		apsw.word[apsw.len + i] = apsw.word[i];
	if (strcmp(apsw.word, bpsw.word) == 0) return true;
	return false;
}

void special(Mypsw apsw, Mypsw bpsw)
{
	for (int i = 0; i < 5; i++) {
		if (Capital(apsw, bpsw, i)) {
			capitalp[i] += 1;
			break;
		}
	}
	if (Repeat(apsw, bpsw))
		repeatp += 1;
}

void train(Mypsw apsw, Mypsw bpsw)
{
	float abdis = edit_dis(apsw.word, bpsw.word);
	if (abdis < PSWSIM_THRESHOLD) {
		BSlen[apsw.BS.length] += 1;
		Mypsw atobpsw = apsw;
		delseg(atobpsw, bpsw);
		BSlenforadd[atobpsw.BS.length] += 1;
		swapseg(atobpsw);
		addseg(atobpsw, bpsw);
		for (int i = 0; i < atobpsw.BS.length; i++) {
			if (!atobpsw.BS.word[i].origflag) continue;
			seglen[atobpsw.BS.word[i].num] += 1;
			delchar(atobpsw.BS.word[i], bpsw.BS.word[i]);
			addchar(atobpsw.BS.word[i], bpsw.BS.word[i]);
		}
		atobpsw.updateword();
	}
	special(apsw, bpsw);
}

void read_file()
{
	ifstream train_file;
	train_file.open(FILE_TRAINING);
	int lineCount = 0;
	char line[MAXLINE];
	while (train_file.getline(line, MAXLINE)) {
		lineCount++;
		if (lineCount % 10000 == 0) printf("%d\n", lineCount);
		int l = strlen(line), pswnum = 0;
		if (line[l - 1] == '\n') { line[l - 1] = '\0'; l--; }
		if (l == 0) continue;
		memset(password, 0, sizeof(password));
		for (int i = 0, j = 0; i < l && pswnum < 3; i++) {
			if (line[i] == '\t') {
				j = 0;
				pswnum += 1;
				continue;
			}
			password[pswnum].word[j++] = line[i];
		}
		if (pswnum != 2) continue;
		totalpswnum += 1;
		int toolongflag = 0;
		for (int i = 1; i < 3; i++) {
			password[i].len = strlen(password[i].word);
			if (password[i].len >= MAX_WORD_LENGTH) {
				toolongflag = 1;
				break;
			}
			password[i].updateBS();
		}
		if (toolongflag) continue;
		train(password[1], password[2]);
		/*
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (i == j) continue;
				train(password[i], password[j]);
			}
		}
		*/
	}
	printf("trainfile linecount: %d\n", lineCount);
	train_file.close();
}

void out_model()
{
	ofstream out_file;
	out_file.open(FILE_OUT);
	int delsegsum = 0, addsegsum = 0, swapsegsum = 0;
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		BSlen[0] += BSlen[i];
		seglen[0] += seglen[i];
		swapsegsum += swapsegp[i];
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 1; j < MAX_WORD_LENGTH; j++) {
			delseglenp[i][0] += delseglenp[i][j];
			addseglenp[i][0] += addseglenp[i][j];
		}
		delsegsum += delseglenp[i][0];
		addsegsum += addseglenp[i][0];
	}
	out_file << "#delsegposp" << endl;
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		for (int j = 0; j < MAX_WORD_LENGTH; j++) {
			float tmpro=0.0;
			if (BSlen[i]) tmpro = (float)delsegposp[i][j] / BSlen[i];
			out_file << tmpro << '\t';
		}
		out_file << endl;
	}
	out_file << "#delseglenp" << endl;
	for (int i = 0; i < 3; i++) {
		out_file << (float)delseglenp[i][0] / delsegsum << '\t';
		for (int j = 1; j < MAX_WORD_LENGTH; j++)
			out_file << (float)delseglenp[i][j] / delseglenp[i][0] << '\t';
		out_file << endl;
	}
	out_file << "#addsegposp" << endl;
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		for (int j = 0; j < MAX_WORD_LENGTH; j++) {
			float tmpro = 0.0;
			if (BSlenforadd[i]) tmpro = (float)addsegposp[i][j] / BSlenforadd[i];
			out_file << tmpro << '\t';
		}
		out_file << endl;
	};
	out_file << "#addseglenp" << endl;
	for (int i = 0; i < 3; i++) {
		out_file << (float)addseglenp[i][0] / addsegsum << '\t';
		for (int j = 1; j < MAX_WORD_LENGTH; j++)
			out_file << (float)addseglenp[i][j] / addseglenp[i][0] << '\t';
		out_file << endl;
	}
	out_file << "#swapsegp" << endl;
	for (int i = 0; i < MAX_WORD_LENGTH; i++)
		out_file << (float)swapsegp[i] / swapsegsum << '\t';
	out_file << endl;
	out_file << "#delcharp" << endl;
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		for (int j = 0; j < MAX_WORD_LENGTH; j++) {
			float tmpro = 0.0;
			if (seglen[i]) tmpro = (float)delcharp[i][j] / seglen[i];
			out_file << tmpro << '\t';
		}
		out_file << endl;
	}
	out_file << "#addcharp" << endl;
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		for (int j = 0; j < MAX_WORD_LENGTH; j++) {
			float tmpro = 0.0;
			if (seglenforadd[i]) tmpro = (float)addcharp[i][j] / seglenforadd[i];
			out_file << tmpro << '\t';
		}
		out_file << endl;
	}
	out_file << "#capitalp" << endl;
	for (int i = 0; i < 5; i++)
		out_file << (float)capitalp[i] / totalpswnum << "\t";
	out_file << endl;
	out_file << "#repeatp" << endl;
	out_file << (float)repeatp / totalpswnum;
	out_file.close();
}

int main()
{
	read_file();
	out_model();
	system("pause");
}