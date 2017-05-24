#include <vector>
#include <iostream>
#include <algorithm>
#include <set>
#include <map>
#include <assert.h>

using namespace std;

struct Solver {
	int rows, cols;
	vector<int> groupSizes;
	int personCount;
	int freeSpaces;
	int totalSpaces;
	int reservedSpaces;
	vector<int> places;
	vector<int> flattenedGroups;
	vector<int> group2school;

	void read() {
		cin >> rows >> cols;
		cin >> reservedSpaces;

		int schools, programs;
		cin >> schools;
		cin >> programs;
		group2school = vector<int>(programs);
		for (auto& v : group2school) cin >> v;

		groupSizes = vector<int>(programs);
		cin >> personCount;
		for (int i = 0; i < personCount; i++) {
			int program;
			cin >> program;
			groupSizes[program]++;
		}

		for (int i = 0; i < programs; i++) cerr << groupSizes[i] << " ";

		totalSpaces = rows*cols;
		freeSpaces = totalSpaces - personCount - reservedSpaces;
		if (freeSpaces < 0) {
			cerr << "Not enough places for everyone" << endl;
			exit(1);
		}

		cerr << "Free spaces " << freeSpaces << endl;
		cerr << "Groups: " << groupSizes.size() << endl;
		cerr << "Persons: " << personCount << endl;

		places = vector<int>(rows*cols, -1);

		for (int g = 0; g < (int)groupSizes.size(); g++) {
			for (int i = 0; i < groupSizes[g]; i++) {
				flattenedGroups.push_back(g);
			}
		}
	}

	void solve() {
		search(0, 0, freeSpaces);
	}

	int rowLength (int row) {
		return row == rows - 1 ? cols - reservedSpaces : cols;
	}

	void search(int index, int placed, int remainingSpaces) {
		if (index == (int)flattenedGroups.size()) {
			if (remainingSpaces == 0) {
				for (; placed < (int)places.size(); placed++) places[placed] = -2;
				evaluatePlacement();
			}
			return;
		}

		for (int s = 0; s <= 1; s++) {
			places[placed] = flattenedGroups[index];
			int nPlaced = placed + 1;
			if (s == 1) {
				int columnIndex = nPlaced % cols;
				int rowIndex = nPlaced / cols;
				int remainingSpacesInRow = rowLength(rowIndex) - columnIndex;
				if (remainingSpacesInRow <= remainingSpaces) {
					for (int i = 0; i < remainingSpacesInRow; i++) places[nPlaced + i] = -1;
					nPlaced += remainingSpacesInRow;

					int nRemainingSpaces = remainingSpaces - remainingSpacesInRow;
					search(index + 1, nPlaced, nRemainingSpaces);
				}
			} else {
				search(index + 1, nPlaced, remainingSpaces);
			}
		}
	}

	int nextProcessionGroupSplitPoint(int place) {
		for (int i = place + 1; i < (int)places.size(); i++) {
			// Check for group split
			if (i > 0 && places[i-1] != places[i]) {
				return i;
			}

			// Check for end of line
			if (i % cols == 0) {
				return i;
			}
		}

		return (int)places.size();
	}

	struct Token {
		int start, end;
		bool left, right;
	};

	void tokenizeForProcessionGroups(vector<Token>& result) {
		int start = 0;
		while(true) {
			int end = nextProcessionGroupSplitPoint(start);
			if (end == start) break;
			if (places[start] >= 0) {
				Token tok;
				tok.start = start;
				tok.end = end;
				tok.left = (start % cols) == 0;
				tok.right = (end % cols) == 0;
				result.push_back(tok);
			}
			start = end;
		}
	}

	struct ProcessionGroup {
		int start, end;
		bool mustBeLeft;
		bool mustBeRight;
	};

	void searchProcessionGroup(int start, vector<ProcessionGroup>& processionGroups, vector<Token>& tokens) {
		if (start == (int)tokens.size()) {
			if ((int)processionGroups.size() != 9) return;

			evaluateProcessionGroups(processionGroups, tokens);
			return;
		}

		if ((int)processionGroups.size() >= 9) return;

		for (int end = start + 1; end <= (int)tokens.size(); end++) {
			auto groupSize = tokens[end-1].end - tokens[start].start;

			// Limit group size (mostly for performance)
			if (groupSize > 50) break;

			// Limit group size
			if (groupSize < 10) break;

			ProcessionGroup group;
			group.start = start;
			group.end = end;

			// Last token has the reserved places on the right side
			// so they must enter from the left
			group.mustBeLeft = end == (int)tokens.size();
			// The group cannot pass through already filled places to its left
			group.mustBeRight = !tokens[start].left;

			// Filter out conflicting constraints
			if (group.mustBeLeft && group.mustBeRight) continue;


			processionGroups.push_back(group);
			// Start new empty group at the current position
			searchProcessionGroup(end, processionGroups, tokens);
			processionGroups.pop_back();
		}
	}

	void printPlacement(vector<int>& place2group, vector<bool>& side) {

		for (int r = rows - 1; r >= 0; r--) {
			if (place2group[r*cols+0] != -1 && side[place2group[r*cols+0]] == false) {
				cerr << " -> ";
			} else {
				cerr << "    ";
			}

			int lastValidIndex = 0;

			for (int c = 0; c < cols; c++) {
				int index = r*cols + c;
				int p = places[index];
				int spaces = 4;
				if (p == -1) {
					cerr << "  ";
				} else if (p == -2) {
					cerr << " X";
				} else {
					assert(place2group[index] != -1);
					intToColor(place2group[index]+1).print();
					cerr << " ";
					spaces--;
					cerr << group2school[p];
					spaces--;
					cerr << "/";
					spaces -= (p >= 10 ? 1 : 0);
					cerr << p;
					lastValidIndex = index;
				}
				for (int i = 0; i < spaces; i++) cerr << " ";
				resetColor();
				//cerr << "\t";
			}

			if (place2group[r*cols+0] != -1 && side[place2group[lastValidIndex]] == true) {
				cerr << " <- ";
			} else {
				cerr << "    ";
			}

			cerr << endl;
		}
	}


	void calculateProcessionOrder(vector<int>& place2group, vector<bool>& groupSide) {
		vector<vector<pair<int,pair<int,int>>>> procession (groupSide.size());
		int index = 0;
		for (int i = 0; i < (int)places.size(); i++) {
			if (place2group[i] != -1) {
				int row = i / cols;
				int sideBonus = groupSide[place2group[i]] ? i : -i;
				procession[place2group[i]].push_back(pair<int,pair<int,int>>(-row*100000 + sideBonus, make_pair(index, i)));
				index++;
			}
		}

		for (int i = 0; i < groupSide.size(); i++) {
			auto& ls = procession[i];
			sort(ls.begin(), ls.end());
			reverse(ls.begin(), ls.end());

			cout << endl;
			cout << "Group " << (i+1) << " " << (groupSide[i] ? "right" : "left") << " " << ls.size() << endl;
			while(ls.size() > 0) {
				int row = ls.back().second.second / cols;
				vector<int> currentCol;
				while(ls.back().second.second / cols == row) {
					currentCol.push_back(ls.back().second.first);
					ls.pop_back();

					if (ls.size() == 0 || currentCol.size() == 3) {
						break;
					}
				}

				if (groupSide[i]) reverse(currentCol.begin(), currentCol.end());
				for (auto c : currentCol) cout << c << " ";
				cout << endl;
			}
		}
	}

	struct Color {
	    int r, g, b;

	    void print() {
			cerr << "\x1b[" << 48 << ";2;" << r << ";" << g << ";" << b << "m";	    	
	    }
	};

	void resetColor() {
		cerr << "\033[0m";
	}

	int bit(int i, int index) {
	    return (i >> index) & 1;
	}

	Color intToColor (int i) {
	    int r = bit(i, 1) + bit(i, 3) * 2 + 1;
	    int g = bit(i, 2) + bit(i, 4) * 2 + 1;
	    int b = bit(i, 0) + bit(i, 5) * 2 + 1;
	    return Color { (int)(255*r*0.25),(int)(255*g*0.25),(int)(255*b*0.25) };
	}

	int c = 0;
	long long c2 = 0;
	void evaluatePlacement() {
		//cerr << "*" << endl;
		c++;

		if ((c % 100) == 0) {
			cerr << "\r" << c << ", " << c2 << "              ";
			cerr.flush();
		}

		vector<Token> tokens;
		tokenizeForProcessionGroups(tokens);
		/*printPlacement();
		cerr << "Tokens: " << tokens.size() << endl;
		for (int i = 0; i < tokens.size(); i++) {
			for (int j = tokens[i].first; j < tokens[i].second; j++) {
				cerr << places[j] << " ";
			}
			cerr << endl;
		}*/

		vector<ProcessionGroup> processionGroups;
		searchProcessionGroup(0, processionGroups, tokens);

		// printPlacement();
		
		// Determine procession groups

	}

	int bestScore = 1000000;
	vector<pair<int,int>> bestProcessionGroups;
	vector<pair<int,int>> bestTokens;

	void evaluateProcessionGroups(vector<ProcessionGroup>& processionGroups, vector<Token>& tokens) {
		//cout << "+";
		//c2++;

		int programScore = 0;
		int schoolScore = 0;
		int sizeScore = 0;
		for (auto processionGroup : processionGroups) {
			int included = 0;
			int includedSchools = 0;
			int includedCount = 0;
			int includedSchoolCount = 0;
			int groupSize = 0;
			for (int tokenIndex = processionGroup.start; tokenIndex < processionGroup.end; tokenIndex++) {
				auto start = tokens[tokenIndex].start;
				auto end = tokens[tokenIndex].end;
				groupSize += end - start;
				if ((included >> places[start] & 1) == 0) {
					included |= 1 << places[start];
					includedCount++;
				}

				if ((includedSchools >> group2school[places[start]] & 1) == 0) {
					includedSchools |= 1 << group2school[places[start]];
					includedSchoolCount++;
				}
			}
			programScore += includedCount*includedCount*1;
			schoolScore += includedSchoolCount*includedSchoolCount*1000;
			sizeScore += groupSize*groupSize*5;
		}

		int baseScore = programScore + schoolScore + sizeScore;

		int bound = 1 << processionGroups.size();
		bool anyWorked = false;
		vector<bool> bestSide(processionGroups.size());
		int orderScore = 10000000;
		for (int order = 0; order < bound; order++) {
			c2++;
			bool worked = true;
			for(int i = 0; i < (int)processionGroups.size(); i++) {
				bool rightSide = ((order >> i) & 0x1) != 0;
				auto group = processionGroups[i];
				if (group.mustBeLeft && rightSide) worked = false;
				if (group.mustBeRight && !rightSide) worked = false;
			}

			if (worked) {
				int innerScore = 0;
				vector<bool> sideList(processionGroups.size());
				for(int i = 0; i < (int)processionGroups.size(); i++) {
					bool rightSide = ((order >> i) & 0x1) != 0;
					sideList[i] = rightSide;
				}

				for (int i = 0; i < (int)sideList.size() - 1; i++) {
					if (sideList[i] == sideList[i+1]) {
						innerScore += 2000;
					}
				}

				if (innerScore < orderScore) {
					orderScore = innerScore;
					bestSide = sideList;
				}
			}

			anyWorked |= worked;
		}

		if (!anyWorked) return;

		int score = baseScore + orderScore;
		if (score < bestScore) {
			bestScore = score;
			cerr << endl;
			cerr << "Best score: " << score << "           " << endl;
			cerr << "   Program: " << programScore << endl;
			cerr << "    School: " << schoolScore << endl;
			cerr << "      Size: " << sizeScore << endl;
			cerr << "     Order: " << orderScore << endl;
			cerr << endl;
			cerr << "Number of groups " << processionGroups.size() << endl;
			vector<int> place2group(places.size(), -1);
			for (int g = 0; g < (int)processionGroups.size(); g++) {
				auto processionGroup = processionGroups[g];
				for (int tokenIndex = processionGroup.start; tokenIndex < processionGroup.end; tokenIndex++) {
					for (int p = tokens[tokenIndex].start; p < tokens[tokenIndex].end; p++) {
						place2group[p] = g;
					}
				}
			}

			printPlacement(place2group, bestSide);
			calculateProcessionOrder(place2group, bestSide);
			for (int i = 0; i < (int)processionGroups.size(); i++) {
				intToColor(i + 1).print();
				cerr << "Group " << i << " enters from the " << (bestSide[i] ? "right" : "left");
				resetColor();
				cerr << endl;
			}
		}
	}
};

int main () {
	Solver solver;
	solver.read();
	solver.solve();
}