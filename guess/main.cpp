#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<fstream>
#include<time.h>
#include<algorithm>
#include<vector>
#include<map>
#include<queue>
#include<math.h>
#include<stdio.h>
#include <sstream>
#include "BaseStructure.h"
#include "alphaNumPair.h"
using namespace std;

#define FILE_TOP "../data/top_psw.txt" //TOP口令文件
#define FILE_MODEL "../data/email_sina_csdn_reuse_multi_unique_model.txt" //训练模型文件
#define FILE_TEST "../data/email_163_csdn_reuse_multi_unique.txt" //训练集文件路径
#define FILE_PCFG "../data/PCFG_data.txt"
#define FILE_MARKOV "../data/markov_data.txt"
#define FILE_RMARKOV "../data/markov_reverse_data.txt"
#define FILE_OUT "../data/out.txt"
#define FILE_FAIL "../data/fail.txt"
#define FILE_START "../data/start_file.txt"

#define MAXLINE 100000
#define PSWSIM_THRESHOLD 0.9
#define SEGSIM_THRESHOLD 0.4
#define MARKOV_ORDER 3
#define GUESS_NUM 100

#define DELSEG_STEP 0
#define SWAPSEG_STEP 1
#define ADDSEG_STEP 2
#define DELCHAR_STEP 3
#define ADDCHAR_STEP 4
#define SPECIAL_STEP 5
#define COMPLETE_STEP 6

class Toppsw
{
public:
	char word[MAX_WORD_LENGTH] = { 0 };
	double prob = 0.0;
};

bool topcmp(Toppsw a, Toppsw b)
{
	return a.prob > b.prob;
}

class Mypsw
{
public:
	char word[MAX_WORD_LENGTH] = { 0 };
	int len;
	BaseStructure BS;
	int step;
	float prob = 0.0;
	void updateBS();
	void printBS();
	void updateword();
	void swaptwoseg(int x, int y);
	void insertseg(int index, char *str);
	void delemptychar();
	void capitalize(int type);
	void repeat();
	bool operator<(const Mypsw& b) const {
		if (step != b.step)
			return step < b.step;
		return prob < b.prob;
	}
};
Mypsw password[3];
Toppsw toplist[10010];
int toppswnum = 0;
float delsegposp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
float delseglenp[3][MAX_WORD_LENGTH] = { 0 };
float addsegposp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
float addseglenp[3][MAX_WORD_LENGTH] = { 0 };
float swapsegp[MAX_WORD_LENGTH] = { 0 };
float delcharp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
float addcharp[MAX_WORD_LENGTH][MAX_WORD_LENGTH] = { 0 };
float capitalp[5];
float repeatp = 0;
int DLSnum[3][12];
vector<Toppsw> DLSseg[3][12];
map<string, float* > markov_pro;
map<string, float* > markov_reverse_pro;
char st = 1;//start symbol
ofstream out_file;
ofstream fail_file;
int totalpswnum = 0;
int crackpswnum = 0;
int crackguessnum[GUESS_NUM];
int diffpswnum = 0;
int topcracknum = 0;
int topguessnum[GUESS_NUM];
int startpos = 0;

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
		printf("%c%d:%s nowpos:%d aimpos:%d\n", BS.word[i].ch, BS.word[i].num, BS.word[i].segment,BS.word[i].nowpos,BS.word[i].aimpos);
	printf("\n");
}

void Mypsw::updateword()
{
	memset(word, 0, sizeof(word));
	len = 0;
	for (int i = 0; i < BS.length; i++) {
		if (BS.word[i].delflag) {
			BS.word[i].delflag = false;
			continue;
		}
		for (int j = 0; j < BS.word[i].num; j++) {
			word[len++] = BS.word[i].segment[j];
		}
	}
}

void Mypsw::swaptwoseg(int x, int y)
{
	alphaNumPair tmpnp = BS.word[x];
	BS.word[x] = BS.word[y];
	BS.word[y] = tmpnp;
	BS.word[x].swapflag = true;
	updateword();
}

void Mypsw::insertseg(int index, char *str)
{
	for (int i = BS.length - 1; i >= index; i--)
		BS.word[i + 1] = BS.word[i];
	alphaNumPair tmpnp;
	tmpnp.origflag = true;
	tmpnp.ch = char2alpha(str[0]);
	tmpnp.num = strlen(str);
	strcpy(tmpnp.segment, str);
	BS.word[index] = tmpnp;
	BS.length += 1;
	updateword();
}

void Mypsw::delemptychar()
{
	for (int i = 0; i < BS.length; i++) {
		char tmpstr[MAX_WORD_LENGTH] = { 0 };
		int newnum = 0;
		for (int j = 0; j < BS.word[i].num; j++) {
			if (BS.word[i].segment[j] == 0) continue;
			tmpstr[newnum] = BS.word[i].segment[j];
			BS.word[i].charpos[newnum] = newnum;
			newnum += 1;
		}
		strcpy(BS.word[i].segment, tmpstr);
		BS.word[i].num = newnum;
	}
}

void Mypsw::capitalize(int type)
{
	for (int i = 0; i < BS.length; i++) { //type:0-首字母大写，1-全大写，2-全小写,3-首字母小写，4-全小写后首字母大写
		if (BS.word[i].ch != 'L') continue;
		if (type == 0) {
			if (BS.word[i].segment[0] >= 'a'&&BS.word[i].segment[0] <= 'z')
				BS.word[i].segment[0] -= 32;
		}
		else if (type == 1) {
			for (int j = 0; j < BS.word[i].num; j++) {
				if (BS.word[i].segment[j] >= 'a'&&BS.word[i].segment[j] <= 'z')
					BS.word[i].segment[j] -= 32;
			}
		}
		else if (type == 2) {
			for (int j = 0; j < BS.word[i].num; j++) {
				if (BS.word[i].segment[j] >= 'A'&&BS.word[i].segment[j] <= 'Z')
					BS.word[i].segment[j] += 32;
			}
		}
		else if (type == 3) {
			if (BS.word[i].segment[0] >= 'A'&&BS.word[i].segment[0] <= 'Z')
				BS.word[i].segment[0] += 32;
		}
		else if (type == 4) {
			for (int j = 0; j < BS.word[i].num; j++) {
				if (BS.word[i].segment[j] >= 'A'&&BS.word[i].segment[j] <= 'Z')
					BS.word[i].segment[j] += 32;
			}
			BS.word[i].segment[0] -= 32;
		}
	}
	updateword();
}

void Mypsw::repeat()
{
	for (int i = 0; i < len; i++)
		word[len + i] = word[i];
	len *= 2;
	updateBS();
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

void prepare_markov(float *aimarr, string pre_seq, string nxt_seq, string segpre_seq, string segnxt_seq)
{
	for (int i = 0; i < 100; i++)
		aimarr[i] = 0;
	if (markov_pro.find(pre_seq) != markov_pro.end()) {
		float *tmpfarr = markov_pro[pre_seq];
		for (int i = 0; i < 100; i++)
			aimarr[i] = max(aimarr[i], tmpfarr[i]);
	}
	if (markov_pro.find(segpre_seq) != markov_pro.end()) {
		float *tmpfarr = markov_pro[segpre_seq];
		for (int i = 0; i < 100; i++)
			aimarr[i] = max(aimarr[i], tmpfarr[i]);
	}
	if (markov_reverse_pro.find(nxt_seq) != markov_reverse_pro.end()) {
		float *tmpfarr = markov_reverse_pro[nxt_seq];
		for (int i = 0; i < 100; i++)
			aimarr[i] = max(aimarr[i], tmpfarr[i]);
	}
	if (markov_reverse_pro.find(segnxt_seq) != markov_reverse_pro.end()) {
		float *tmpfarr = markov_reverse_pro[segnxt_seq];
		for (int i = 0; i < 100; i++)
			aimarr[i] = max(aimarr[i], tmpfarr[i]);
	}
	bool allzeroflag = true;
	for (int i = 0; i < 100; i++)
		if (aimarr[i] != 0) {
			allzeroflag = false;
			break;
		}
	if (!allzeroflag) return;
	for (int i = 0; i < 100; i++)
		aimarr[i] = 1.0 / 95.0;
}

bool isvalid(char *pswstr)
{
	if (strlen(pswstr) < 8) return false;
	return true;
}

void genguess(Mypsw psw, vector<Toppsw>& guesslist)
{
	float prothreshold = 0.000001;
	float listminprob = 1.0;
	int listminindex = 0;
	map<string, int> indexmap;
	psw.updateBS();
	psw.prob = 1.0;
	psw.step = DELSEG_STEP;
	priority_queue<Mypsw> genq;
	genq.push(psw);
	psw.step = SPECIAL_STEP;
	genq.push(psw);
	while (!genq.empty()) {
		Mypsw curpsw = genq.top();
		genq.pop();
		int curBSlen = curpsw.BS.length;
		if (curpsw.prob < prothreshold) continue;
		if (curpsw.step == DELSEG_STEP) {
			int start = 0, restsegnum = 0;
			for (int i = curBSlen - 1; i >= 0; i--) {
				if (start == 0 && curpsw.BS.word[i].delflag)
					start = i + 1;
				if (!curpsw.BS.word[i].delflag) restsegnum += 1;
			}
			for (int i = start; restsegnum > 1 && i < curBSlen; i++) {
				int typeindex = alpha2index(curpsw.BS.word[i].ch);
				//float segtypepro = delseglenp[typeindex][0] * delseglenp[typeindex][curpsw.BS.word[i].num];
				float seglenpro = delsegposp[curBSlen][i];
				float delpro = curpsw.prob*seglenpro;
				if (delpro < prothreshold) continue;
				Mypsw tmppsw = curpsw;
				tmppsw.prob = delpro;
				tmppsw.BS.word[i].delflag = true;
				genq.push(tmppsw);
			}
			curpsw.step = SWAPSEG_STEP;
			curpsw.updateword();
			curpsw.updateBS();
			genq.push(curpsw);
		}
		else if (curpsw.step == SWAPSEG_STEP) {
			int start = 0;
			for (int i = curBSlen - 1; i >= 0; i--)
				if (curpsw.BS.word[i].swapflag) {
					start = i + 1;
					break;
				}
			for (int i = start; i < curBSlen - 1; i++) {
				float swappro = swapsegp[1] * curpsw.prob;
				if (swappro < prothreshold) continue;
				Mypsw tmppsw = curpsw;
				tmppsw.prob = swappro;
				tmppsw.swaptwoseg(i, i + 1);
				genq.push(tmppsw);
			}
			curpsw.step = ADDSEG_STEP;
			for (int i = 0; i < curpsw.BS.length; i++)
				curpsw.BS.word[i].nowpos = i;
			genq.push(curpsw);
		}
		else if (curpsw.step == ADDSEG_STEP) {
			int start = 0, inisegnum = 0;
			for (int i = curBSlen - 1; i >= 0; i--) {
				if (start == 0 && curpsw.BS.word[i].origflag)
					start = i + 1;
				if (!curpsw.BS.word[i].origflag) inisegnum += 1;
			}
			for (int i = start; i <= curBSlen; i++) {
				float pospro = curpsw.prob;
				if (i == curBSlen)
					pospro *= addsegposp[inisegnum][inisegnum];
				else
					pospro *= addsegposp[inisegnum][curpsw.BS.word[i].nowpos];
				if (pospro < prothreshold) continue;
				for (int type = 0; type < 3; type++) {
					float typepro = pospro * addseglenp[type][0];
					for (int j = 1; j <= 10; j++) {
						float lenpro = typepro * addseglenp[type][j];
						for (Toppsw tmptop : DLSseg[type][j]) {
							float newpro = lenpro * tmptop.prob;
							if (newpro < prothreshold) continue;
							Mypsw tmppsw = curpsw;
							tmppsw.prob = newpro;
							tmppsw.insertseg(i, tmptop.word);
							genq.push(tmppsw);
						}
					}
				}
			}
			curpsw.step = DELCHAR_STEP;
			genq.push(curpsw);
		}
		else if (curpsw.step == DELCHAR_STEP) {
			int startseg = 0, startpos = 0;
			for (int i = curBSlen - 1; i >= 0; i--) {
				if (curpsw.BS.word[i].origflag) continue;
				for (int j = curpsw.BS.word[i].num - 1; j >= 0; j--)
					if (curpsw.BS.word[i].segment[j] == 0) {
						startseg = i;
						startpos = j + 1;
						break;
					}
				if (startseg != 0 || startpos != 0) break;
			}
			for (int i = startseg; i < curBSlen; i++, startpos = 0) {
				if (curpsw.BS.word[i].origflag) continue;
				int restcharnum = 0;
				for (int j = 0; j < curpsw.BS.word[i].num; j++)
					if (curpsw.BS.word[i].segment[j] != 0) restcharnum += 1;
				if (restcharnum <= 1) continue;
				for (int j = (i == startseg) ? startpos : 0; j < curpsw.BS.word[i].num; j++) {
					float newpro = curpsw.prob*delcharp[curpsw.BS.word[i].num][j];
					if (newpro < prothreshold) continue;
					Mypsw tmppsw = curpsw;
					tmppsw.prob = newpro;
					tmppsw.BS.word[i].segment[j] = 0;
					genq.push(tmppsw);
				}
			}
			curpsw.step = ADDCHAR_STEP;
			curpsw.delemptychar();
			genq.push(curpsw);
		}
		else if (curpsw.step == ADDCHAR_STEP) {
			int startseg = 0, startpos = 0;
			for (int i = curBSlen - 1; i >= 0; i--) {
				if (curpsw.BS.word[i].origflag) continue;
				for (int j = curpsw.BS.word[i].num - 1; j >= 0; j--)
					if (curpsw.BS.word[i].charpos[j] == -1) {
						startseg = i;
						startpos = j + 1;
						break;
					}
				if (startseg != 0 || startpos != 0) break;
			}
			string tmp_seg = curpsw.word;
			for (int i = 0; i < MARKOV_ORDER; i++) {
				tmp_seg = tmp_seg + st;
				tmp_seg = st + tmp_seg;
			}
			int acclen = 0;
			for (int i = 0; i < startseg; i++)
				acclen += curpsw.BS.word[i].num;
			for (int i = startseg; i < curBSlen; acclen+=curpsw.BS.word[i].num, i++, startpos = 0) {
				if (curpsw.BS.word[i].origflag) continue;
				int iniseglen = 0;
				for (int j = 0; j < curpsw.BS.word[i].num; j++)
					if (curpsw.BS.word[i].charpos[j] != -1) iniseglen += 1;
				for (int j = (i == startseg) ? startpos : 0; j <= curpsw.BS.word[i].num; j++) {
					int insertpos = (j == curpsw.BS.word[i].num) ? iniseglen : curpsw.BS.word[i].charpos[j];
					float newpro = curpsw.prob*addcharp[iniseglen][insertpos];
					if (newpro < prothreshold) continue;
					string pre_seq, nxt_seq, segpre_seq, segnxt_seq, segtmp_seg;
					pre_seq = tmp_seg.substr(acclen + j, MARKOV_ORDER);
					nxt_seq = tmp_seg.substr(acclen + j + MARKOV_ORDER, MARKOV_ORDER);
					segtmp_seg = curpsw.BS.word[i].segment;
					for (int i = 0; i < MARKOV_ORDER; i++) {
						segtmp_seg = segtmp_seg + st;
						segtmp_seg = st + segtmp_seg;
					}
					segpre_seq = segtmp_seg.substr(j, MARKOV_ORDER);
					segnxt_seq = segtmp_seg.substr(j + MARKOV_ORDER, MARKOV_ORDER);
					float tmpcharpro[100];
					prepare_markov(tmpcharpro, pre_seq, nxt_seq, segpre_seq, segnxt_seq);
					Mypsw tmppsw = curpsw;
					for (int k = curpsw.BS.word[i].num - 1; k >= j; k--) {
						tmppsw.BS.word[i].segment[k + 1] = tmppsw.BS.word[i].segment[k];
						tmppsw.BS.word[i].charpos[k + 1] = tmppsw.BS.word[i].charpos[k];
					}
					tmppsw.BS.word[i].charpos[j] = -1;
					tmppsw.BS.word[i].num += 1;
					for (int k = 0; k < 95; k++) {
						float addcharpro = newpro * tmpcharpro[k];
						if (addcharpro < prothreshold) continue;
						tmppsw.BS.word[i].segment[j] = k + 32;
						tmppsw.prob = addcharpro;
						tmppsw.updateword();
						genq.push(tmppsw);
					}
				}
			}
			curpsw.step = COMPLETE_STEP;
			genq.push(curpsw);
		}
		else if (curpsw.step == SPECIAL_STEP) {
			for (int i = 0; i < 5; i++) {
				float newpro = curpsw.prob*capitalp[i];
				if (newpro < prothreshold) continue;
				Mypsw tmppsw = curpsw;
				tmppsw.prob = newpro;
				tmppsw.step = COMPLETE_STEP;
				tmppsw.capitalize(i);
				genq.push(tmppsw);
				tmppsw.step = DELSEG_STEP;
				genq.push(tmppsw);
			}
			if (2 * curpsw.len >= MAX_WORD_LENGTH) continue;
			float newpro = curpsw.prob*repeatp;
			if (newpro < prothreshold) continue;
			Mypsw tmppsw = curpsw;
			tmppsw.prob = newpro;
			tmppsw.step = COMPLETE_STEP;
			tmppsw.repeat();
			genq.push(tmppsw);
		}
		else if (curpsw.step == COMPLETE_STEP) {
			if (!isvalid(curpsw.word) && curpsw.prob < 1) continue;
			//out_file << curpsw.word << "\t" << curpsw.prob << "\t" << curpsw.step << endl;
			string tmpstring = curpsw.word;
			if (indexmap.find(tmpstring) != indexmap.end()) {
				int tmpindex = indexmap[tmpstring];
				if (curpsw.prob > guesslist[tmpindex].prob) {
					if (listminindex == tmpindex) {
						guesslist[tmpindex].prob = curpsw.prob;
						listminprob = 1.0;
						for (int i = 0; i < guesslist.size(); i++) {
							if (guesslist[i].prob < listminprob) {
								listminprob = guesslist[i].prob;
								listminindex = i;
							}
						}
					}
					guesslist[tmpindex].prob = curpsw.prob;
				}
			}
			else if (guesslist.size() < GUESS_NUM) {
				Toppsw tmptp;
				tmptp.prob = curpsw.prob;
				strcpy(tmptp.word, curpsw.word);
				guesslist.push_back(tmptp);
				if (curpsw.prob < listminprob) {
					listminprob = curpsw.prob;
					listminindex = guesslist.size() - 1;
				}
				indexmap[tmpstring] = guesslist.size() - 1;
				if (guesslist.size() == GUESS_NUM)
					prothreshold = listminprob;
			}
			else if (curpsw.prob > listminprob) {
				string delstr = guesslist[listminindex].word;
				indexmap.erase(delstr);
				indexmap[tmpstring] = listminindex;
				guesslist[listminindex].prob = curpsw.prob;
				strcpy(guesslist[listminindex].word, curpsw.word);
				listminprob = 1.0;
				for (int i = 0; i < guesslist.size();i++) {
					if (guesslist[i].prob < listminprob) {
						listminprob = guesslist[i].prob;
						listminindex = i;
					}
				}
				prothreshold = listminprob;
			}
			else {
				//out_file << "抛弃\t" << prothreshold << endl;
			}
		}
	}
}

void read_testfile()
{
	ifstream test_file;
	test_file.open(FILE_TEST);
	int lineCount = 0;
	char line[MAXLINE];
	while (test_file.getline(line, MAXLINE)) {
		lineCount++;
		if (lineCount % 100000 == 0) printf("%d\n", lineCount);
		int l = strlen(line), pswnum = 0;
		if (line[l - 1] == '\n') { line[l - 1] = '\0'; l--; }
		if (l == 0) continue;
		memset(password, 0, sizeof(password));
		bool toolongflag = false;
		for (int i = 0, j = 0; i < l && pswnum < 3; i++) {
			if (line[i] == '\t') {
				j = 0;
				pswnum += 1;
				continue;
			}
			if (j >= MAX_WORD_LENGTH) {
				toolongflag = true;
				break;
			}
			password[pswnum].word[j++] = line[i];
		}
		if (toolongflag) continue;
		if (pswnum != 2) continue;
		totalpswnum += 1;
		if (totalpswnum < startpos) continue;
		/*
		if (strcmp(password[1].word, password[2].word) != 0) {
			diffpswnum += 1;
			for (int i = 0; i < GUESS_NUM; i++) {
				if (strcmp(toplist[i].word, password[2].word) == 0) {
					topcracknum += 1;
					topguessnum[i] += 1;
					break;
				}
			}
		}
		*/
		for (int i = 0; i < 3; i++) {
			password[i].len = strlen(password[i].word);
			password[i].updateBS();
		}
		vector<Toppsw> guesslist;
		genguess(password[1], guesslist);
		for (int i = 0; i < GUESS_NUM; i++)
			guesslist.push_back(toplist[i]);
		sort(guesslist.begin(), guesslist.end(), topcmp);
		bool failflag = true;
		for (int i = 0; i < min(GUESS_NUM, (int)guesslist.size()); i++) {
			if (strcmp(guesslist[i].word, password[2].word) == 0) {
				crackguessnum[i] += 1;
				crackpswnum += 1;
				failflag = false;
				break;
			}
		}
		if (failflag)
			fail_file << password[1].word << "\t" << password[2].word << endl;
		
		if (totalpswnum % 100 == 0) {
			printf("crack rate:%f\t%d/%d\n", (float)crackpswnum / totalpswnum, crackpswnum, totalpswnum);
			//printf("top creack rate:%f\t%d/%d\n", (float)topcracknum / diffpswnum, topcracknum, diffpswnum);
			printf("\n");
		}
		if (totalpswnum % 1000 == 0) {
			out_file << "crack rate:\t" << (float)crackpswnum / totalpswnum << "\t" << crackpswnum << "/" << totalpswnum << endl;
		}
	}
	out_file << "total crack rate:\t" << (float)crackpswnum / totalpswnum << "\t" << crackpswnum << "/" << totalpswnum << endl;
	for (int i = 0; i < GUESS_NUM; i++) {
		out_file << i + 1 << "\t" << crackguessnum[i] << endl;
	}
	out_file << endl;
	for (int i = 0; i < GUESS_NUM; i++) {
		//out_file << i + 1 << "\t" << topguessnum[i] << "\t" << toplist[i].word << "\t" << toplist[i].prob << endl;
	}
	printf("testfile linecount: %d\n", lineCount);
	test_file.close();
}

void read_Markovfile()
{
	ifstream markov_file;
	markov_file.open(FILE_MARKOV);
	string line, pre;
	while (getline(markov_file, line)) {
		if (line[0] == '\t') {
			pre = line.substr(1, MARKOV_ORDER);
			markov_pro[pre] = new float[100];
		}
		else {
			char next = line[0];
			float pro = atof(line.substr(2, line.length() - 2).c_str());
			markov_pro[pre][next - 32] = pro;
		}
	}
	printf("markov file end\n");
	markov_file.close();
	markov_file.open(FILE_RMARKOV);
	while (getline(markov_file, line)) {
		if (line[0] == '\t') {
			pre = line.substr(1, MARKOV_ORDER);
			markov_reverse_pro[pre] = new float[100];
		}
		else {
			char next = line[0];
			float pro = atof(line.substr(2, line.length() - 2).c_str());
			markov_reverse_pro[pre][next - 32] = pro;
		}
	}
	printf("reverse markov file end\n");
	markov_file.close();
}

void read_PCFGfile()
{
	ifstream PCFG_file;
	PCFG_file.open(FILE_PCFG);
	char line[MAXLINE];
	for (int i = 0; i < 3; i++) {
		char tmpc;
		int tmpsum;
		PCFG_file >> tmpc >> tmpsum;
		DLSnum[alpha2index(tmpc)][0] = tmpsum;
	}
	int type,slen;
	while (PCFG_file.getline(line, MAXLINE)) {
		if (strlen(line) == 0) continue;
		stringstream ssin(line);
		if (line[0] == '\t') {
			char ch;
			int num;
			ssin >> ch >> slen >> num;
			type = alpha2index(ch);
			DLSnum[type][slen] = num;
		}
		else {
			Toppsw tmpseg;
			ssin >> tmpseg.word >> tmpseg.prob;
			DLSseg[type][slen].push_back(tmpseg);
		}
	}
	printf("PCFG file end\n");
	PCFG_file.close();
}

void read_topfile()
{
	ifstream top_file;
	top_file.open(FILE_TOP);
	int lineCount = 0;
	char line[MAXLINE];
	while (top_file.getline(line, MAXLINE)) {
		stringstream ssin(line);
		ssin >> toplist[toppswnum].word >> toplist[toppswnum].prob;
		if (!isvalid(toplist[toppswnum].word)) continue;
		toppswnum += 1;
		lineCount += 1;
	}
	sort(toplist, toplist + toppswnum, topcmp);
	printf("topfile linecount: %d\n", lineCount);
	top_file.close();
}

void read_model()
{
	ifstream model_file;
	model_file.open(FILE_MODEL);
	char line[MAXLINE];
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> delsegposp[i][j];
	}
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < 3; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> delseglenp[i][j];
	}
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> addsegposp[i][j];
	}
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < 3; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> addseglenp[i][j];
	}
	model_file.getline(line, MAXLINE);
	model_file.getline(line, MAXLINE);
	stringstream sssin(line);
	for (int i = 0; i < MAX_WORD_LENGTH; i++)
		sssin >> swapsegp[i];
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> delcharp[i][j];
	}
	model_file.getline(line, MAXLINE);
	for (int i = 0; i < MAX_WORD_LENGTH; i++) {
		model_file.getline(line, MAXLINE);
		stringstream ssin(line);
		for (int j = 0; j < MAX_WORD_LENGTH; j++)
			ssin >> addcharp[i][j];
	}
	model_file.getline(line, MAXLINE);
	model_file.getline(line, MAXLINE);
	stringstream ssssin(line);
	for (int i = 0; i < 5; i++)
		ssssin >> capitalp[i];
	model_file.getline(line, MAXLINE);
	model_file.getline(line, MAXLINE);
	repeatp = atof(line);
	printf("model file end\n");
	model_file.close();
}

void read_start()
{
	ifstream start_file;
	start_file.open(FILE_START);
	char line[MAXLINE];
	start_file.getline(line, MAXLINE);
	startpos = atoi(line);
	start_file.getline(line, MAXLINE);
	crackpswnum = atoi(line);
	for (int i = 0; i < GUESS_NUM; i++) {
		start_file.getline(line, MAXLINE);
		crackguessnum[i] = atoi(line);
	}
}

void test_one(char *pswstr)
{
	Mypsw newpsw;
	newpsw.len = strlen(pswstr);
	strcpy(newpsw.word, pswstr);
	newpsw.updateBS();
	vector<Toppsw> guesslist;
	genguess(newpsw, guesslist);
	sort(guesslist.begin(), guesslist.end(), topcmp);
	for (int i = 0; i < guesslist.size(); i++) {
		printf("%s\t%f\n", guesslist[i].word, guesslist[i].prob);
	}
}

int main()
{
	out_file.open(FILE_OUT,ios::app);
	fail_file.open(FILE_FAIL,ios::app);
	//read_start();
	read_model();
	read_topfile();
	read_PCFGfile();
	read_Markovfile();
	read_testfile();
	/*
	while (true) {
		char tmps[MAX_WORD_LENGTH];
		scanf("%s", tmps);
		test_one(tmps);
	}
	*/
	system("pause");
}