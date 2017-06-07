#include <vector>
#include <iostream>
#include <algorithm>
#include <set>
#include <map>
#include <assert.h>
#include <chrono>
#include <thread>

using namespace std;

// A very large number
#define INF 1000000000

struct Token {
	int start, end;
	bool left, right;
};

struct ProcessionGroup {
	int start, end;
	// The procession group 'must' enter from the left (e.g due to using the same row as the reserved seats)
	bool mustBeLeft;
	// The procession group 'must' enter from the right
	bool mustBeRight;
};

struct WeightConfig {
	double programWeight = 1;
	double schoolWeight = 1;
	double sizeWeight = 1;

	// Best score so far for this configuration of weights
	// Note that we are trying to minimize this score
	double bestScore = INF;
	vector<ProcessionGroup> bestProcessionGroups;
	vector<Token> bestTokens;
	vector<bool> bestSide;
	vector<int> places;
};

/** Represents an RGB color */
struct Color {
    int r, g, b;

    void print() {
		cerr << "\x1b[" << 48 << ";2;" << r << ";" << g << ";" << b << "m";	    	
    }
};

struct Solver {
	// Number of rows and columns of seats
	int rows, cols;
	vector<int> groupSizes;
	// Total number of persons
	int personCount;
	// Number of seats that will be left empty
	int freeSpaces;
	// Total number of seats (rows*cols)
	int totalSpaces;
	// Number of seats that are reserved
	// The reserved seats are at the first row to the right
	int reservedSpaces;
	// Index of the person sitting at each seat
	// or -1 if there is no person there
	// or -2 if the seat is reserved
	vector<int> places;
	vector<int> flattenedGroups;
	// Maps a group index 'i' to a school index 'v' using
	// group2school[i] == v
	vector<int> group2school;

	vector<WeightConfig> weights;

	bool partial = true;

	double programWeightScale = 112;
	double schoolWeightScale = 588;
	double sizeWeightScale = 1.35;
	double orderWeightScale = 2000;

	double orderWeight = 1;
	int numProcessionGroups;

	vector<Color> colors = {{31,120,180},{51,160,44},{53,88,237},{218,0,37},{106,61,154},{218,149,0}};

	int minimumGroupSize = 18;
	int maximumGroupSize = 50;

	void read() {
		cin >> rows >> cols;
		cin >> reservedSpaces;
		cin >> numProcessionGroups;

		if (numProcessionGroups <= 0) {
			cerr << "Too few procession groups (" << numProcessionGroups << ")" << endl;
			exit(1);
		}

		// Some bitshifting tricks require that we don't use too many bits.
		// Also too many groups will be extremely slow anyway.
		if (numProcessionGroups >= 30) {
			cerr << "Too many procession groups (" << numProcessionGroups << " >= 30)" << endl;
			exit(1);
		}

		int numWeightConfigs;
		cin >> numWeightConfigs;
		if (numWeightConfigs <= 0) {
			cerr << "No score weights were given in the configuration" << endl;
			exit(1);
		}

		weights = vector<WeightConfig>(numWeightConfigs);
		for(auto& w : weights) {
			cin >> w.programWeight;
			cin >> w.schoolWeight;
			cin >> w.sizeWeight;
		}

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

		// Debug group sizes
		// for (int i = 0; i < programs; i++) cerr << groupSizes[i] << " ";

		if (reservedSpaces > cols) {
			cerr << "More than 1 row reserved. Reduce the number of rows instead" << endl;
			cerr << "(" << reservedSpaces << " seats reserved, and there are " << cols << " seats per row)" << endl;
			exit(1);
		}

		totalSpaces = rows*cols;
		freeSpaces = totalSpaces - personCount - reservedSpaces;

		cerr << "Free spaces: " << freeSpaces << endl;
		cerr << "Groups: " << groupSizes.size() << endl;
		cerr << "Persons: " << personCount << endl;

		if (freeSpaces < 0) {
			cerr << "Not enough places for everyone" << endl;
			exit(1);
		}

		places = vector<int>(rows*cols, -1);

		for (int g = 0; g < (int)groupSizes.size(); g++) {
			for (int i = 0; i < groupSizes[g]; i++) {
				flattenedGroups.push_back(g);
			}
		}
	}

	void solve() {
		search(0, 0, freeSpaces);
		for (int i = 0; i < 3; i++) {
			if (weights[0].bestScore == INF) {
				cerr << endl << "Found no solutions! Relaxing constraints..." << endl;
				minimumGroupSize = (2*minimumGroupSize) / 3;
				search(0, 0, freeSpaces);
			}
		}
	}

	void printSolution() {
		if (weights.size() > 0) {
			cerr << endl;
			if (weights[0].bestScore == INF) {
				cerr << "Found no solutions!" << endl;
				exit(1);
			}

			for (auto& w : weights) {
				printBestResult(w, false);
			}
		}
	}

	/** Length of a particular row while taking into account reserved seats */
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

	/** Next seat index at which a procession group can start.
	 * Starting from the seat index defined by the 'place' parameter.
	 */
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

	void searchProcessionGroup(int start, vector<ProcessionGroup>& processionGroups, vector<Token>& tokens) {
		if (start == (int)tokens.size()) {
			if ((int)processionGroups.size() == numProcessionGroups) {
				// Found a valid configuration
				evaluateProcessionGroups(processionGroups, tokens);
			}
			return;
		}

		// If we have only 1 procession group left add, go directly to the end since we know
		// this group must contain everything that hasn't already been covered.
		int searchStart = (int)processionGroups.size() == numProcessionGroups - 1 ? (int)tokens.size() : start + 1;
		for (int end = searchStart; end <= (int)tokens.size(); end++) {
			auto groupSize = tokens[end-1].end - tokens[start].start;

			// Limit group size (mostly for performance)
			if (groupSize > maximumGroupSize) break;

			// Limit group size
			if (groupSize < minimumGroupSize) continue;

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

	/** Pretty print a seating configuration.
	 * Works best in terminals that support 24bit colors.
	 *
	 * place2group maps each seat to the procession group it is in, this value will be converted to a color.
	 * side maps each group to which side it enters from (false for left, true for right) and shows this using
	 * arrows.
	 */
	void printPlacement(const vector<int>& place2group, const vector<bool>& side, const vector<int>& seats) {

		for (int r = rows - 1; r >= 0; r--) {
			if (place2group[r*cols+0] != -1 && side[place2group[r*cols+0]] == false) {
				cerr << " -> ";
			} else {
				cerr << "    ";
			}

			int lastValidIndex = 0;

			for (int c = 0; c < cols; c++) {
				int index = r*cols + c;
				int p = seats[index];
				int spaces = 4;
				if (p == -1) {
					cerr << "   ";
					spaces--;
				} else if (p == -2) {
					cerr << "  X";
					spaces--;
				} else {
					assert(place2group[index] != -1);
					intToColor(place2group[index]).print();
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
			}

			if (place2group[r*cols+0] != -1 && side[place2group[lastValidIndex]] == true) {
				cerr << " <- ";
			} else {
				cerr << "    ";
			}

			cerr << endl;
		}
	}


	void printProcessionOrder(const vector<int>& place2group, const vector<bool>& groupSide) {
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

		for (int i = 0; i < (int)groupSide.size(); i++) {
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

	/** Resets the terminal's color information */
	void resetColor() {
		cerr << "\033[0m";
	}

	/** Extracts bit 'index' from 'v' */
	int bit(int v, int index) {
	    return (v >> index) & 1;
	}

	/** Converts an integer to a nice color value */
	Color intToColor (int i) {
		return colors[i % colors.size()];

		// Alternative colors
	    //int r = bit(i, 1) + bit(i, 3) * 2 + 1;
	    //int g = bit(i, 2) + bit(i, 4) * 2 + 1;
	    //int b = bit(i, 0) + bit(i, 5) * 2 + 1;
	    //return Color { (int)(255*r*0.25),(int)(255*g*0.25),(int)(255*b*0.25) };
	}

	int evaluatedPlacements = 0;
	long long evaluations = 0;
	long long notWorking = 0;
	void evaluatePlacement() {
		evaluatedPlacements++;

		if ((evaluatedPlacements % 100) == 0) {
			// Print the number of evaluations regularly
			// We need to add some whitespace afterwards to make sure to clear any existing text on the same line
			cerr << "\r" << evaluatedPlacements << " " << evaluations << " " << notWorking << "              ";
			cerr.flush();
		}

		// Split the seating configuration into a number of 'tokens' which are indivisible components
		// that the procession groups will be made of
		vector<Token> tokens;
		tokenizeForProcessionGroups(tokens);

		if ((evaluatedPlacements % 1) == 0) {
			//debugTokens(tokens);
		}

		// Try all possible procession groups
		vector<ProcessionGroup> processionGroups;
		searchProcessionGroup(0, processionGroups, tokens);
	}

	void debugTokens(vector<Token>& tokens) {
		// Move curser to the top right and clear the screen
		cerr << "\033[1;1H" << "\033[J";
		auto v = vector<int>(places.size(), 0);
		for (int k = 0; k < (int)tokens.size(); k++) {
			auto& tok = tokens[k];
			for (int i = tok.start; i < tok.end; i++) {
				v[i] = k;
			}
		}
		printPlacement(v, vector<bool>(1, false), places);
		cerr.flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//exit(1);
	}

	/** Square a value */
	int sqr(int x) {
		return x*x;
	}

	struct BestSideCalculator {
		vector<ProcessionGroup>& processionGroups;
		int bestScore = INF;
		int bestSides = 0;
		int sides = 0;
		int score = 0;

		BestSideCalculator(vector<ProcessionGroup>& processionGroups) : processionGroups(processionGroups) {}

		void calculateBestSideConfiguration(int index) {
			if(score >= bestScore) return;

			if(index == (int)processionGroups.size()) {
				bestScore = score;
				bestSides = sides;
				return;
			}

			auto& group = processionGroups[index];
			if(group.mustBeLeft && group.mustBeRight) return;
			
			// Try both left and right
			// For index=0 this will just be 0
			int side = (sides >> (index - 1)) & 0x1;
			side ^= 1;
			if ((!group.mustBeRight || side == 1) && (!group.mustBeLeft || side == 0)) {
				sides = (sides & ~(1 << index)) | (side << index);
				calculateBestSideConfiguration(index + 1);
			}

			side ^= 1;
			if ((!group.mustBeRight || side == 1) && (!group.mustBeLeft || side == 0)) {
				if(index > 0) score++;
				sides = (sides & ~(1 << index)) | (side << index);
				calculateBestSideConfiguration(index + 1);
				if(index > 0) score--;
			}
		}
	};

	void evaluateProcessionGroups(vector<ProcessionGroup>& processionGroups, vector<Token>& tokens) {
		double programScore = 0;
		double schoolScore = 0;
		double sizeScore = 0;
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
			programScore += sqr(includedCount);
			schoolScore += sqr(includedSchoolCount);
			sizeScore += sqr(groupSize);
		}

		double emptySeatScore = 0;
		for (int r = 0; r < rows; r++) {
			int lastFilledSeat = rowLength(r) - 1;
			while(places[r*cols + lastFilledSeat] < 0 && lastFilledSeat > 0) lastFilledSeat--;
			emptySeatScore += sqr(rowLength(r) - lastFilledSeat - 1);
		}

		bool anyWorked = false;
		vector<bool> side;
		double orderScore = 10000000;

		bool slow = false;
		if (slow) {
			int bound = 1 << processionGroups.size();
			bool anyWorked = false;
			int bestOrder = 0;
			vector<bool> side(processionGroups.size());
			for (int order = 0; order < bound; order++) {
				evaluations++;
				bool worked = true;
				for(int i = 0; i < (int)processionGroups.size(); i++) {
					bool rightSide = ((order >> i) & 0x1) != 0;
					auto group = processionGroups[i];
					if (group.mustBeLeft && rightSide) worked = false;
					if (group.mustBeRight && !rightSide) worked = false;
				}

				if (worked) {
					notWorking++;
					//int numSameSide = __builtin_popcount((order ^ (~order >> 1)) & (bound/2 - 1));
					// This is the equivalent code of the bit-hack above
					// It just checks how many groups enter from the same side as the group after it
					
					int numSameSide = 0;
					for(int i = 0; i < (int)processionGroups.size() - 1; i++) {
						bool rightSide1 = ((order >> i) & 0x1) != 0;
						bool rightSide2 = ((order >> (i+1)) & 0x1) != 0;
						if (rightSide1 == rightSide2) {
							numSameSide += 1;
						}
					}
					double innerScore = numSameSide;// * orderWeight * orderWeightScale;

					if (innerScore < orderScore) {
						orderScore = innerScore;

						vector<bool> sideList(processionGroups.size());
						for(int i = 0; i < (int)processionGroups.size(); i++) {
							bool rightSide = ((order >> i) & 0x1) != 0;
							sideList[i] = rightSide;
						}
						side = sideList;
						bestOrder = order;
					}
				}

				anyWorked |= worked;
			}
		} else {

			BestSideCalculator sideCalculator(processionGroups);
			sideCalculator.calculateBestSideConfiguration(0);
			anyWorked = sideCalculator.bestScore != INF;

			side = vector<bool>(processionGroups.size());
			orderScore = sideCalculator.bestScore * orderWeight * orderWeightScale;
			for(int i = 0; i < (int)processionGroups.size(); i++) {
				bool rightSide = ((sideCalculator.bestSides >> i) & 0x1) != 0;
				side[i] = rightSide;
			}
		}

		if (!anyWorked) {
			return;
		}

		/*if (sideCalculator.bestScore != orderScore) {
			cerr << "! " << sideCalculator.bestScore << " " << orderScore << endl;
			cerr << sideCalculator.bestSides << " " << bestOrder << endl;
			exit(1);
		}*/


		for (auto& w : weights) {
			auto finalProgramScore = programScore * w.programWeight * programWeightScale;
			auto finalSchoolScore = schoolScore * w.schoolWeight * schoolWeightScale;
			auto finalSizeScore = sizeScore * w.sizeWeight * sizeWeightScale;
			auto finalOrderScore = orderScore;
			double score = finalProgramScore + finalSchoolScore + finalSizeScore + finalOrderScore + emptySeatScore;

			if (score < w.bestScore) {
				w.bestScore = score;

				w.bestProcessionGroups = processionGroups;
				w.bestSide = side;
				w.bestTokens = tokens;
				w.places = places;

				if (partial) {
					cerr << endl;
					cerr << "Best score: " << (int)score << "           " << endl;
					cerr << "Empty Seat: " << (int)emptySeatScore << endl;
					cerr << "   Program: " << (int)finalProgramScore << endl;
					cerr << "    School: " << (int)finalSchoolScore << endl;
					cerr << "      Size: " << (int)finalSizeScore << endl;
					cerr << "     Order: " << (int)finalOrderScore << endl;
					cerr << endl;
					printBestResult(w, true);
				}
			}
		}
	}

	void printBestResult(const WeightConfig& w, bool onlyDebug) {
		vector<int> place2group(places.size(), -1);
		for (int g = 0; g < (int)w.bestProcessionGroups.size(); g++) {
			auto processionGroup = w.bestProcessionGroups[g];
			for (int tokenIndex = processionGroup.start; tokenIndex < processionGroup.end; tokenIndex++) {
				for (int p = w.bestTokens[tokenIndex].start; p < w.bestTokens[tokenIndex].end; p++) {
					place2group[p] = g;
				}
			}
		}

		printPlacement(place2group, w.bestSide, w.places);
		if (!onlyDebug) {
			printProcessionOrder(place2group, w.bestSide);
		}

		/*for (int i = 0; i < (int)w.bestProcessionGroups.size(); i++) {
			intToColor(i).print();
			cerr << "Group " << i << " enters from the " << (w.bestSide[i] ? "right" : "left");
			resetColor();
			cerr << endl;
		}*/
	}
};

int main () {
	Solver solver;
	solver.read();
	solver.solve();
	solver.printSolution();
}