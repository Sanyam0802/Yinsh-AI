#include<iostream>
#include<sstream>
#include<string>
#include<vector>
#include<iterator>
#include<algorithm>

static double inf = 100000; // infinity to be put into beta
static double minf = -100000; // infinity to be put into alpha
using namespace std;

static int player_id;
static int board_size;
static int game_timer;
static int numberOfRings = 5;
static int ** board;

static int turn; // Keeps track of whose turn is this (0 is this player and 1 is other player)
static int depth; // Keeps track of depth for the search
static int previousTurn; // Keeps track of the previous turn

typedef struct Coord { // Stores hex - value pairs
	Coord(int hex,int pos): hex(hex),pos(pos) {}
	Coord() {}
	int hex;
	int pos;
	bool operator ==(const Coord& a) {
		return (this->hex == a.hex and this->pos == a.pos);
	}

} Coord;
typedef struct Move { // Stores move received from the client of other player (Not used anywhere else)
	Move(int player,string moveType,int hex,int pos) : player(player),moveType(moveType),hex(hex),pos(pos) {}
	int player;
	string moveType;
	int hex;
	int pos;
} Move;
typedef struct RowInfo { // Stores any row found in the board
	RowInfo(Coord start,Coord end,int direction,int player):start(start),end(end),direction(direction),player(player) {}
	Coord start;
	Coord end;
	int direction;
	int player;
	bool operator==(const RowInfo& a) {
		return (this->direction == a.direction and this->player == a.player and this->start == a.start and this->end == a.end);
	}
} RowInfo;
typedef struct Removal { // Stores info about an removal - remove rows and remove a ring
	Removal(RowInfo rowRemoval,Coord ringRemoval):rowRemoval(rowRemoval),ringRemoval(ringRemoval) {}
	RowInfo rowRemoval;
	Coord ringRemoval;
	bool operator==(const Removal& a) {
		return (this->ringRemoval == a.ringRemoval and this->rowRemoval == a.rowRemoval);
	}
} Removal;
typedef struct MoveInfo { // Stores info about a ring movement
	MoveInfo(Coord start,Coord end,int direction):start(start),end(end),direction(direction) {}
	MoveInfo() {
		start = Coord(-1,-1);
		end = Coord(-1,-1);
		direction = -1;
	}
	Coord start;
	Coord end;
	int direction; // Direction to move
	bool operator ==(const MoveInfo& a) {
		return (this->direction == a.direction and this->start == a.start and this->end == a.end);
	}
} MoveInfo;
typedef struct Ply { // Is a ply (Either just a move) or (A sequence of row removals)
	Ply(MoveInfo moveInfo):moveInfo(moveInfo) {
		vector<Removal> rowRemovals;
		flag = 0;
	}
	Ply() {
		vector<Removal> rowRemovals;
		flag = -1;
	}
	Ply(Coord ringPlacement):ringPlacement(ringPlacement) {
		vector<Removal> rowRemovals;
		flag = 1;
	}
	bool operator==(const Ply &b) {
		if(this->flag != b.flag) {
			return 0;
		} else {
			if(this->flag == -1) {
				if(this->rowRemovals.size() != b.rowRemovals.size()) {
					return 0;
				}
				for(int i=0; i<this->rowRemovals.size(); i++) {
					if(!(this->rowRemovals[i] == b.rowRemovals[i])) {
						return 0;
					}
				}
				return 1;
			}
			if(b.flag == 0) {
				return this->moveInfo == b.moveInfo;
			} else {
				return this->ringPlacement == b.ringPlacement;
			}
		}
	}
	MoveInfo moveInfo;
	Coord ringPlacement;
	vector <Removal> rowRemovals;
	int flag;
} Ply;
typedef struct Node { // GameTree nodes
	Node(Ply plyTaken) {
		vector<Node*> neighbors;
		plyTaken = plyTaken;
		vector <RowInfo> myRows;
		vector <RowInfo> otherRows;
	}
	Node() {
		vector<Node*> neighbors;
		vector <RowInfo> otherRows;
		vector <RowInfo> myRows;
	}
	vector<Node*> neighbors; // Its neighbor(children)
	Ply plyTaken; // Ply taken from its parent to reach this node
	double value; // Backed up value
	vector<RowInfo> myRows; // If rows of this player
	vector<RowInfo> otherRows; // Rows of the opposite player
	bool explored; // If it is explored , will speed up process
} Node;
typedef struct RemovalInfo {
	RemovalInfo(Removal removal,int sortingFactor):removal(removal),sortingFactor(sortingFactor) {}
	Removal removal;
	int sortingFactor;
} RemovalInfo;
static Node* GameTree;
bool comparePermutations(RemovalInfo &a,RemovalInfo &b) { // Utility function
	return (a.sortingFactor < b.sortingFactor);
}

class Player {
	public:
		int id;
		int totalRings; // in A2 this is 5, A5 can be 6 or 7
		int ringsOnBoard; // number of rings on board at current time
		vector <Coord> positions; // list of ring positions
		Player() { // Constructor
			this->ringsOnBoard = 0;
		}
		void placeRing(int hex,int pos) { // Places rings on the board
			positions.push_back(Coord(hex,pos));
			ringsOnBoard++;
		}
		void removeRing(int hex,int pos) { // remove rings given a position
			vector<Coord> ::iterator i;
			for(i=positions.begin(); i!=positions.end(); i++) {
				if((*i).hex == hex and (*i).pos == pos) {
					break;
				}
			}
			positions.erase(i);
			ringsOnBoard--;
		}
};
static Player player1 = Player(); // Initiate players
static Player player2 = Player(); // Initiate players
Coord moveSouth(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0 :
				return Coord(current.hex-1,current.pos);
			case 1:
				return Coord(current.hex,current.pos+1);
			case 2:
				return Coord(current.hex+1,current.pos+3);
			case 3:
				return Coord(current.hex+1,current.pos+3);
			case 4:
				return Coord(current.hex+1,current.pos+3);
			case 5:
				return Coord(current.hex,current.pos-1);
		}
	} else {
		switch(segment) {
			case 0:
				return Coord(current.hex-1,current.pos);
			case 1:
				return Coord(current.hex,current.pos+1);
			case 2:
			case 3:
				return Coord(current.hex+1,current.pos+3);
			case 4:
				return Coord(current.hex,current.pos-1);
			case 5:
				return Coord(current.hex-1,current.pos-6);
		}
	}
}
Coord moveNorthEast(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0 :
			case 1 :
			case 2 :
				return Coord(current.hex+1,current.pos+1);
			case 3:
				return Coord(current.hex,current.pos-1);
			case 4:
				return Coord(current.hex-1,current.pos-4);
			case 5:
				return Coord(current.hex,current.pos+1);
		}
	} else {
		switch(segment) {
			case 0 :
			case 1 :
				return Coord(current.hex+1,current.pos+1);
			case 2:
				return Coord(current.hex,current.pos-1);
			case 3:
			case 4 :
				return Coord(current.hex-1,current.pos-4);
			case 5:
				return Coord(current.hex,current.pos+1);
		}
	}
}
Coord moveNorthWest(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0:
				return Coord(current.hex+1,6*current.hex+5);
			case 1:
				return Coord(current.hex,current.pos-1);
			case 2:
				return Coord(current.hex-1,current.pos-2);
			case 3:
				return Coord(current.hex,current.pos+1);
			case 4:
			case 5:
				return Coord(current.hex+1,current.pos+1);
		}
	} else {
		switch(segment) {
			case 0 :
				return Coord(current.hex,current.pos-1);
			case 1 :
			case 2:
				return Coord(current.hex-1,current.pos-2);
			case 3 :
				return Coord(current.hex,current.pos+1);
			case 4 :
			case 5:
				return Coord(current.hex+1,current.pos+5);
		}
	}
}
Coord moveSouthEast(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0:
				return Coord(current.hex,current.pos+1);
			case 1 :
			case 2 :
			case 3:
				return Coord(current.hex+1,current.pos+2);
			case 4:
				return Coord(current.hex,current.pos-1);
			case 5:
				return Coord(current.hex-1,current.pos-5);
		}
	} else {
		switch(segment) {
			case 0:
				return Coord(current.hex,current.pos+1);
			case 1 or 2:
				return Coord(current.hex+1,current.pos+2);
			case 3 :
				return Coord(current.hex,current.pos-1);
			case 4:
				return Coord(current.hex-1,current.pos-5);
			case 5:
				return Coord(current.hex-1,(current.pos-5)%current.hex);
		}
	}
}
Coord moveSouthWest(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0:
				return Coord(current.hex,(current.pos-1)%current.hex);
			case 1:
				return Coord(current.hex-1,current.pos-1);
			case 2:
				return Coord(current.hex,current.pos+1);
			case 3 :
			case 4 :
			case 5:
				return Coord(current.hex+1,current.pos+4);
		}
	} else {
		switch(segment) {
			case 0 :
			case 1 :
				return Coord(current.hex-1,current.pos-1);
			case 2:
				return Coord(current.hex,current.pos-1);
			case 3 :
			case 4 :
				return Coord(current.hex+1,current.pos+4);
			case 5:
				return Coord(current.hex,current.pos-1);
		}
	}
}
Coord moveNorth(Coord current) {
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	if(shift == 0) {
		switch(segment) {
			case 0 :
			case 1:
				return Coord(current.hex+1,current.pos);
			case 2:
				return Coord(current.hex,current.pos-1);
			case 3:
				return Coord(current.hex-1,current.pos-3);
			case 4:
				return Coord(current.hex,current.pos+1);
			case 5:
				return Coord(current.hex+1,current.pos+6);
		}
	} else {
		switch(segment) {
			case 0:
				return Coord(current.hex+1,current.pos);
			case 1:
				return Coord(current.hex,current.pos-1);
			case 2 :
			case 3:
				return Coord(current.hex-1,current.pos-3);
			case 4:
				return Coord(current.hex,current.pos+1);
			case 5:
				return Coord(current.hex+1,current.pos+6);
		}
	}
}
Coord moveDirection(Coord current,int direction) {
	switch(direction) { // Observe how direction is opposite to (direction+3)%6. This is used a lot.
		case 0:
			return moveNorth(current);
		case 1:
			return moveNorthEast(current);
		case 2:
			return moveSouthEast(current);
		case 3:
			return moveSouth(current);
		case 4:
			return moveSouthWest(current);
		case 5:
			return moveNorthWest(current);
	}
}
void placeRing(int player,int hex,int pos) { // Given a hex-pos pair , place the ring on the board and update player
	if(player == 0) {
		board[hex][pos] = 2;
		player1.placeRing(hex,pos);
	} else {
		board[hex][pos] = -2;
		player2.placeRing(hex,pos);
	}
	return;
}
void removeRing(int player,int hex,int pos) { // Remove the ring from the board
	board[hex][pos] = 0;
	if(player == 0) {
		player1.removeRing(hex,pos);
	} else player2.removeRing(hex,pos);
	return;
}
int findLine(int hex1,int pos1,int hex2,int pos2) { // Could be optimized. Finds the direction to reach from one place to other
	Coord start = Coord(hex1,pos1);
	Coord end = Coord(hex2,pos2);
	Coord north = moveNorth(start);
	Coord northEast = moveNorthEast(start);
	Coord southEast = moveSouthEast(start);
	Coord south = moveSouth(start);
	Coord southWest = moveSouthWest(start);
	Coord northWest = moveNorthWest(start);
	bool stop = 0;
	int index;
	while(stop == 0) {
		if(north.hex == hex2 and north.pos == pos2) {
			stop = 1;
			index = 0;
			break;
		}
		north = moveNorth(north);
		if(northEast.hex == hex2 and northEast.pos == pos2) {
			stop = 1;
			index = 1;
			break;
		}
		northEast = moveNorthEast(northEast);
		if(southEast.hex == hex2 and southEast.pos == pos2) {
			stop = 1;
			index = 2;
			break;
		}
		southEast = moveSouthEast(southEast);
		if(south.hex == hex2 and south.pos == pos2) {
			stop = 1;
			index = 3;
			break;
		}
		south = moveSouth(south);
		if(southWest.hex == hex2 and southWest.pos == pos2) {
			stop = 1;
			index = 4;
			break;
		}
		southWest = moveSouthWest(southWest);
		if(northWest.hex == hex2 and northWest.pos == pos2) {
			stop = 1;
			index = 5;
			break;
		}
		northWest = moveNorthWest(northWest);
	}
	return index;
}
void findRow(int direction,Coord temp,vector<RowInfo> &rows) {
	int marker = board[temp.hex][temp.pos]; // Get marker
	int player = (marker == 1)?0:1; // Get player based on marker(whose rows would have been formed)
	int directions[4]; //List of directions to check
	int pos = 0;
	for(int i=0; i<6; i++) {
		if(i == direction or i == direction + 3) { // dont move along the move direction as it will be repeated
			continue;
		}
		directions[pos] = i;
		pos++;
	}
	for(int i=0; i<2; i++) {
		int tempDirection1 = directions[i]; // first direction
		int tempDirection2 = directions[i+2]; //opposite
		Coord rowStart = temp;
		Coord rowEnd = moveDirection(temp,tempDirection2);
		int count1 = 0;
		int count2 = 1;
		while(board[rowStart.hex][rowStart.pos] == marker) {
			rowStart = moveDirection(temp,tempDirection1);
			count1++;
		}
		if(count1 == 5) {
			rows.push_back(RowInfo(rowStart,temp,tempDirection2,player)); // Row Found
		}
		while(board[rowEnd.hex][rowEnd.pos] == marker) {
			count2++;
			rowEnd = moveDirection(temp,tempDirection2);
		}
		if(count2 == 5) {
			rows.push_back(RowInfo(rowEnd,temp,tempDirection1,player)); // Row found
		}
		if(count1+count2>5 and count1<5 and count2<5) { // Row can be found
			Coord newTemp = temp;
			for(int j=0; j<5-count1; j++) {
				newTemp = moveDirection(newTemp,tempDirection2);
			}
			rows.push_back(RowInfo(rowStart,newTemp,tempDirection2,player));
			newTemp = temp;
			for(int j=0; j<5-count2; j++) {
				newTemp = moveDirection(newTemp,tempDirection1);
			}
			rows.push_back(RowInfo(newTemp,rowEnd,tempDirection2,player));
		}
	}
}
void findRowAfterMove(int direction,int hex1,int pos1,int hex2,int pos2,vector<RowInfo> &rows) {
	Coord temp = Coord(hex1,pos1);
	while(temp.hex!=hex2 and temp.pos!=pos2) {
		findRow(direction,temp,rows); // Do it for the 4 directions not along the movement direction
	}
	int marker = board[hex1][pos1];
	int player = (marker == 1)?0:1;
	Coord rowStart = temp;
	Coord rowEnd = temp;
	int count1 = 0;
	while(board[rowStart.hex][rowStart.pos] == marker) {
		count1++;
		rowStart = moveDirection(rowStart,direction);
	}
	if(count1 == 5) {
		rows.push_back(RowInfo(temp,rowStart,direction,player));
	}
	int count2 = 0;
	while(board[rowEnd.hex][rowEnd.pos] == marker) {
		count2++;
		rowEnd = moveDirection(rowEnd,(direction+3)%6);
	}
	if(count2 == 5) {
		rows.push_back(RowInfo(temp,rowEnd,(direction+3)%6,player));
	}
	if(count1 +count2>5 and count1<5 and count2<5) {
		for(int i=0; i<5-count1; i++) {
			temp = moveDirection(temp,(direction+3)%6);
		}
		rows.push_back(RowInfo(temp,rowStart,direction,player));
		temp = Coord(hex1,pos1);
		for(int j=0; j<5-count2; j++) {
			temp = moveDirection(temp,direction);
		}
		rows.push_back(RowInfo(temp,rowEnd,(direction+3)%6,player));
	}
	return;
}
void moveRing(int player,int hex1,int pos1,int hex2,int pos2,vector<RowInfo> &rows,int direction = -1) {
	if(hex1 == hex2 and pos1 == pos2) {
		return;
	}
	if(direction == -1) {
		direction = findLine(hex1,pos1,hex2,pos2);
	}
	int marker = (player == 0)?1:-1;
	int antiMarker = (marker == 1)?-1:1;
	Coord temp = Coord(hex1,pos1);
	while(temp.hex!=hex2 and temp.pos!=pos2) {
		if(board[temp.hex][temp.pos] == marker) { // Flip markers
			board[temp.hex][temp.pos] = antiMarker;
		} else if(board[temp.hex][temp.pos] == antiMarker) {
			board[temp.hex][temp.pos] = marker;
		}
		temp = moveDirection(temp,direction);
	}
	board[hex2][pos2] = (player == 0)?2:-2; // Place ring
	if(player == 0) {
		player1.removeRing(hex1,pos1); // Update player database
		player1.placeRing(hex2,pos2);
	} else {
		player2.removeRing(hex1,pos1);
		player2.placeRing(hex2,pos2);
	}
	findRowAfterMove(direction,hex1,pos1,hex2,pos2,rows);
	return;
}

// The following function could return false when there are intersecting rows. Once you remove one, the other is also destroyed.

bool removeRow(int player,int hex1,int pos1,int hex2,int pos2,int direction = -1) { // Outputs true when the row is removed
	if(direction == -1) {
		int direction = findLine(hex1,pos1,hex2,pos2);
	}
	Coord temp = Coord(hex1,pos1);
	int mark = (player ==0)?1:-1;
	while((temp.hex!=hex2 and temp.pos!=pos2) or board[temp.hex][temp.pos]!=mark) {
		board[temp.hex][temp.pos] == 0;
		temp = moveDirection(temp,direction);
	}
	if(board[temp.hex][temp.pos] !=mark) { // Row not present
		while(temp.hex!=hex1 and temp.pos!=pos1) {
			board[temp.hex][temp.pos] = mark; // Revert back all the changes
			temp = moveDirection(temp,(direction+3)%6);
		}
		board[temp.hex][temp.pos] = mark;
		return 0;
	}
	board[temp.hex][temp.pos] == 0;
	return 1;
}
void executeMoves(vector <Move> moveSequence) { // Execute moves obtained from other player (Only use)
	vector<Move>:: iterator move;
	for(move = moveSequence.begin(); move<moveSequence.end(); move++) {
		string moveType = (*move).moveType;
		vector<RowInfo> rows;
		int player,hex,pos,rowStart1,rowStart2,rowEnd1,rowEnd2;
		if(moveType == "P") {
			placeRing((*move).player,(*move).hex,(*move).pos);
			break;
		}
		if(moveType =="S") {
			player = (*move).player;
			hex = (*move).hex;
			pos = (*move).pos;
			break;
		}
		if(moveType == "M") {
			moveRing(player,hex,pos,(*move).hex,(*move).pos,rows);
			break;
		}
		if(moveType == "RS") {
			rowStart1 = (*move).hex;
			rowStart2 = (*move).pos;
			break;
		}
		if(moveType == "RE") {
			rowEnd1 = (*move).hex;
			rowEnd2 = (*move).pos;
			removeRow((*move).player,rowStart1,rowStart2,rowEnd1,rowEnd2);
			break;
		}
		if(moveType == "X") {
			removeRing((*move).player,(*move).hex,(*move).pos);
			break;
		}
	}
}
void getAvailableMoves(int player,int hex,int pos,vector<MoveInfo> &listOfMoves) { // Get available moves from a start position
	for(int i=0; i<6; i++) {
		Coord temp = moveDirection(Coord(hex,pos),i);
		bool mark = 0;
		while(board[temp.hex][temp.pos]!=2 or board[temp.hex][temp.pos]!=-2 or temp.hex > 5) { // If no ring encountered
			if(mark == 0) { // if no mark encountered yet
				if(board[temp.hex][temp.pos]==1 or board[temp.hex][temp.pos] == -1) { // If encountered
					mark = 1;
				}
			}
			if(mark == 1 and board[temp.hex][temp.pos] == 0) { // If jumped over the marker
				break;
			}
			listOfMoves.push_back(MoveInfo(Coord(hex,pos),temp,i));
			temp = moveDirection(temp,i);
			if(temp.hex == numberOfRings and temp.pos%numberOfRings == 0) { // Reached last hexagon
				break;
			}
		}
	}
}
void selectRings(Player& player,int removeRings,vector<int> &selections) { // From a player's ring database, get a ring selection to remove
	bool count = 0; // RemoveRings is the number of rings to remove
	for(int i=0; i<player.ringsOnBoard and 1<=removeRings; i++) {
		for(int j=i+1; j<player.ringsOnBoard and 2<=removeRings; j++) {
			for(int k=j+1; k<player.ringsOnBoard and 3<=removeRings; k++) {
				for(int l=k+1; l<player.ringsOnBoard and 4<=removeRings; l++) {
					for(int m = l+1; m<player.ringsOnBoard and 5<=removeRings; m++) {
						if(removeRings == 5) {
							selections.push_back(i);
							selections.push_back(j);
							selections.push_back(k);
							selections.push_back(l);
							selections.push_back(m);
						}
					}
					if(removeRings == 4) {
						selections.push_back(i);
						selections.push_back(j);
						selections.push_back(k);
						selections.push_back(l);
					}
				}
				if(removeRings == 3) {
					selections.push_back(i);
					selections.push_back(j);
					selections.push_back(k);
				}
			}
			if(removeRings == 2) {
				selections.push_back(i);
				selections.push_back(j);
			}
		}
		if(removeRings == 1) {
			selections.push_back(i);
		}
	}
}
int executePly(Ply ply,vector<RowInfo> &rows,int turn = -1) { // Given a ply, execute it on the board
	if(ply.flag == 0) { // Movement ply
		int player = (board[ply.moveInfo.start.hex][ply.moveInfo.start.pos] == 2)?0:1;
		moveRing(player,ply.moveInfo.start.hex,ply.moveInfo.start.pos,ply.moveInfo.end.hex,ply.moveInfo.end.pos,rows,ply.moveInfo.direction);
		return 0;
	} else if (ply.flag == -1) { // Ring removal ply
		int m;
		bool removed;
		for(m=0; m<ply.rowRemovals.size() and removed == true; m++) {
			RowInfo rowInfo = ply.rowRemovals[m].rowRemoval;
			removed = removeRow(rowInfo.player,rowInfo.start.hex,rowInfo.start.pos,rowInfo.end.hex,rowInfo.end.pos,rowInfo.direction);
			Coord ringRemoval = ply.rowRemovals[m].ringRemoval;
			if(removed == true) {
				removeRing(rowInfo.player,ringRemoval.hex,ringRemoval.pos);
			}
		}
		return m; // m is the actual number of rows removed(will be less than the rows formed in case of intersecting rows)
	} else {
		placeRing(turn,ply.ringPlacement.hex,ply.ringPlacement.pos); // ring placement
		return 0;
	}
}
void reversePly(Ply &ply,int extent = -1) { // Reverse ply
	if(ply.flag == -1) {
		vector <Removal> :: reverse_iterator i;
		ply.rowRemovals.erase(ply.rowRemovals.begin()+extent,ply.rowRemovals.end()); // Remove the rows which intersect
		for(i=ply.rowRemovals.rbegin(); i!=ply.rowRemovals.rend(); i++) { // Reverse through the removal vector
			RowInfo rowInfo = (*i).rowRemoval;
			Coord ringRemoval = (*i).ringRemoval;
			placeRing(rowInfo.player,ringRemoval.hex,ringRemoval.pos); // Place the ring back
			Coord temp = rowInfo.end;
			int mark = (rowInfo.player == 0)?1:-1;
			while(temp.hex!=rowInfo.start.hex and temp.pos!=rowInfo.start.pos) {
				board[temp.hex][temp.pos] = mark;
				temp = moveDirection(temp,(rowInfo.direction+3)%6); // Add the row back;
			}
		}
		return;
	} else if(ply.flag == 0) { // Reverse ring movement
		MoveInfo move = ply.moveInfo;
		int player = (board[move.end.hex][move.end.pos] == 2)?0:1;
		vector <RowInfo> rows;
		moveRing(player,move.end.hex,move.end.pos,move.start.hex,move.start.pos,rows,move.direction);
		board[move.end.hex][move.end.pos] = 0;
		return;
	} else {
		int player = (board[ply.ringPlacement.hex][ply.ringPlacement.pos] == 2)?0:1;
		removeRing(player,ply.ringPlacement.hex,ply.ringPlacement.pos);
	}
}
void permute(vector<vector<Removal>> &rowRemoval,vector<RowInfo> &rows,vector<int> &selections,Player& player,int removeRings) {
	vector<vector<int>> togetherSelections; // permutations of selections
	vector<int> :: iterator i;
	for(i=selections.begin(); i!=selections.end(); i) { // move through them
		vector<int> temp;
		copy(i,i+removeRings,temp.begin()); // copy one selection
		togetherSelections.push_back(temp); //push it in the selections list
		i = i+removeRings+1;
	}
	selections.clear();
	vector<vector<int>> :: iterator j;
	for(j=togetherSelections.begin(); j!=togetherSelections.end(); j++) { // Loop through the selections
		bool end1 =1;
		vector<int> &tempInt = *j;
		sort(tempInt.begin(),tempInt.end()); // get the first lexicographic permutation
		while(end1 = 1) { // Outer loop for ringSelection permutations
			vector<RemovalInfo> tempRemoval;
			for(int p=0; p<removeRings; p++) {
				tempRemoval.push_back(RemovalInfo(Removal(rows[p],Coord(player.positions[tempInt[p]].hex,player.positions[tempInt[p]].pos)),tempInt[p]));
			} //a temp removal permutation is here
			bool end = 1;
			while(end == 1) { // permutations are available
				vector<Removal> finalRemoval;
				for(int m=0; m<tempRemoval.size(); m++) {
					finalRemoval.push_back(tempRemoval[m].removal);
				}
				rowRemoval.push_back(finalRemoval);
				finalRemoval.clear();
				end = next_permutation(tempRemoval.begin(),tempRemoval.begin(),comparePermutations); // Permute with a given permutation
			}
			end1 = next_permutation(tempInt.begin(),tempInt.end());
		}
	}
	return;
}
double evaluateGameBoard() { //Features to be filled here
	return 0;
}
void compare(double value,double &alpha,double &beta,int turn) { // Utility function for updating alpha beta
	if(turn == 0) { //  MaxNode
		alpha = (value>alpha)?value:alpha;
	} else {
		beta = (value<beta)?value:beta;
	}
}
void change(double& nodeValue,double newValue,int turn) { // Utility function
	if(turn == 0) {
		if(newValue > nodeValue) {
			nodeValue = newValue;
		}
	} else if(turn == 1) {
		if(newValue <nodeValue) {
			nodeValue = newValue;
		}
	}
}
void getRingMovesNot(vector<Coord> &ringPlacement) { // Get Ring placements which cant happen (ring already there)
	for(int i=0; i<player1.ringsOnBoard; i++) {
		ringPlacement.push_back(player1.positions[i]);
	}
	for(int i=0; i<player2.ringsOnBoard; i++) {
		ringPlacement.push_back(player2.positions[i]);
	}
	return;
}
void extendTreeMove(Node *&node,double alpha,double beta,int currDepth);
void extendTree(Node *&node,double alpha,double beta,int currDepth,bool removeRows=0);
void extendTreeRemoval(Node *&node,double alpha,double beta,int currDepth) { // Extend the node by removing rows
	if(node->explored == 1) { // If node was already explored , we dont need to get the list of moves again. Just DFS forward
		double currAlpha = alpha;
		double currBeta = beta;
		vector<RowInfo> rows;
		for(int i=0; i<node->neighbors.size(); i++) {
			executePly(node->neighbors[i]->plyTaken,rows);
			extendTreeMove(node->neighbors[i],currAlpha,currBeta,currDepth);
			compare(node->neighbors[i]->value,currAlpha,currBeta,turn);
			change(node->value,node->neighbors[i]->value,turn);
			rows.clear();
			reversePly(node->neighbors[i]->plyTaken);
			if(currAlpha>currBeta) {
				return;
			}
		}
		return;
	}
	vector<RowInfo> &rows = node->myRows; // get the rows
	vector<Node*> &neighbors = node->neighbors; // reference
	node->value = minf;
	if(turn == 1) { // we have to remove other rows
		rows = node->otherRows;
		node->value = inf;
	}
	Player player = player1;
	if(turn == 1) { //change player
		player = player2;
	}
	double currAlpha = alpha;
	double currBeta = beta;
	int currTurn = turn;
	vector<vector<Removal>> rowRemoval; // will store list of removal sequences
	int removeRings = rows.size(); // number of rows to remove
	vector<int> selections;
	vector<RowInfo> rowsA;
	for(int m=0; m<node->neighbors.size(); m++) { // Some of the nodes already have their plys written. First go through them in the hope we wont have to check further neighbors
		executePly(node->neighbors[m]->plyTaken,rowsA);
		extendTreeMove(node->neighbors[m],currAlpha,currBeta,currDepth);
		compare(node->neighbors[m]->value,currAlpha,currBeta,turn); // Update alpha-beta
		change(node->value,node->neighbors[m]->value,turn); // Back up the value
		//rows.clear();
		reversePly(node->neighbors[m]->plyTaken);
		if(currAlpha>currBeta) {
			return;
		}
	}
	selectRings(player,removeRings,selections); // select rings from the rings on board to remove
	permute(rowRemoval,rows,selections,player,removeRings); // get all row Removal moves
	for(int m=node->neighbors.size(); m<rowRemoval.size(); m++) {
		neighbors.push_back(new Node(Ply()));
		neighbors.back()->plyTaken.rowRemovals = rowRemoval[m]; // update the ply
		if(turn ==0) {
			neighbors.back()->otherRows = node->otherRows; // pass on the rows

		} else neighbors.back()->myRows = node->myRows;
		executePly(neighbors.back()->plyTaken,rows);
		extendTree(neighbors[m],currAlpha,currBeta,currDepth,1);
		compare(neighbors[m]->value,currAlpha,currBeta,turn);
		change(node->value,neighbors[m]->value,turn);
		reversePly(neighbors[m]->plyTaken); // Problem solved
		if(currAlpha>currBeta) {
			return;
		}
	}
	node->explored = 1; // If entire node is explored, we can use this information next time we visit this
}
void extendTreeMove(Node *&node,double alpha,double beta,int currDepth) {
	if(node->explored == 1) { // Same logic
		double currAlpha = alpha;
		double currBeta = beta;
		vector<RowInfo> rows;
		for(int i=0; i<node->neighbors.size(); i++) {
			executePly(node->neighbors[i]->plyTaken,rows);
			extendTreeMove(node->neighbors[i],currAlpha,currBeta,currDepth);
			compare(node->neighbors[i]->value,currAlpha,currBeta,turn);
			change(node->value,node->neighbors[i]->value,turn);
			rows.clear();
			reversePly(node->neighbors[i]->plyTaken);
			if(currAlpha>currBeta) {
				return;
			}
		}
		return;
	}
	Player player;
	if(turn == 0) { // if our player
		player = player1;
	} else player = player2;
	double currAlpha = alpha;
	double currBeta = beta;
	vector<Coord>::iterator i;
	vector<MoveInfo> listOfMoves; // Store the available moves
	vector<RowInfo> rowsFormed; // Will store the rows formed
	for(int m=0; m<node->neighbors.size(); m++) { // Same logic. We have gone down some paths before. Optimize the code
		executePly(node->neighbors[m]->plyTaken,rowsFormed);
		extendTreeMove(node->neighbors[m],currAlpha,currBeta,currDepth);
		compare(node->neighbors[m]->value,currAlpha,currBeta,turn);
		change(node->value,node->neighbors[m]->value,turn);
		rowsFormed.clear();
		reversePly(node->neighbors[m]->plyTaken);
		if(currAlpha>currBeta) {
			return;
		}
	}
	for(i=player.positions.begin(); i!=player.positions.end(); i++) {
		getAvailableMoves(player.id,(*i).hex,(*i).pos,listOfMoves); // get all available moves
	}
	vector<MoveInfo> :: iterator j;
	for(j=listOfMoves.begin()+node->neighbors.size(); j!=listOfMoves.end(); j++) {
		node->neighbors.push_back(new Node((*j))); // Push it in the neighbor
	}//Layer done
	for(int m=node->neighbors.size(); m<node->neighbors.size(); m++) {
		executePly(node->neighbors[m]->plyTaken,rowsFormed); // Execute that ply
		for(int n=0; n<rowsFormed.size(); n++) { // if rows formed
			if(rowsFormed[n].player == turn) {
				node->myRows.push_back(rowsFormed[n]);
			} else node->otherRows.push_back(rowsFormed[n]);
		}
		extendTree(node->neighbors[m],currAlpha,currBeta,currDepth); // Extend node from there
		compare(node->neighbors[m]->value,currAlpha,currBeta,turn); //Update alpha beta
		change(node->value,node->neighbors[m]->value,turn); // Update value
		rowsFormed.clear(); // clear the vector
		reversePly(node->neighbors[m]->plyTaken); // reverse the ply taken
		if(currAlpha>currBeta) {
			return;
		}
	}
	node->explored = 1;
}
void extendTreePlaceRing(Node *&node,double alpha,double beta,int currDepth) { // All these extend functions have same logic
	if(node->explored == 1) {
		double currAlpha = alpha;
		double currBeta = beta;
		vector<RowInfo> rows;
		for(int i=0; i<node->neighbors.size(); i++) {
			executePly(node->neighbors[i]->plyTaken,rows);
			extendTreeMove(node->neighbors[i],currAlpha,currBeta,currDepth);
			compare(node->neighbors[i]->value,currAlpha,currBeta,turn);
			change(node->value,node->neighbors[i]->value,turn);
			rows.clear();
			reversePly(node->neighbors[i]->plyTaken);
			if(currAlpha>currBeta) {
				return;
			}
		}
		return;
	}
	vector<Coord> listOfRingPlaces;
	getRingMovesNot(listOfRingPlaces);
	vector<Coord> ringPlacementMoves;
	double currAlpha = alpha;
	double currBeta = beta;
	vector<RowInfo> rowsFormed;
	for(int m=0; m<node->neighbors.size(); m++) {
		executePly(node->neighbors[m]->plyTaken,rowsFormed);
		extendTreeMove(node->neighbors[m],currAlpha,currBeta,currDepth);
		compare(node->neighbors[m]->value,currAlpha,currBeta,turn);
		change(node->value,node->neighbors[m]->value,turn);
		rowsFormed.clear();
		reversePly(node->neighbors[m]->plyTaken);
		if(currAlpha>currBeta) {
			return;
		}
	}
	for(int i=0; i<5; i++) {
		for(int j=0; j<6*i; j++) {
			if(i == 5) {
				if(j%5 == 0) {
					continue;
				}
			}
			ringPlacementMoves.push_back(Coord(i,j));
		}
	}
	vector<Coord>::iterator j;
	j = ringPlacementMoves.begin();
	for(int i=0; i<listOfRingPlaces.size(); i++) {
		int hex = listOfRingPlaces[i].hex;
		int pos = listOfRingPlaces[i].pos;
		if(hex == 0) {
			ringPlacementMoves.erase(j-i);
		} else ringPlacementMoves.erase(j+ hex*(6*(hex-1))/2 + pos - ((hex == 5)?pos/hex:0) - i);
	}
	listOfRingPlaces.clear();
	for(int i=node->neighbors.size()+1; i<ringPlacementMoves.size(); i++) {
		node->neighbors.push_back(new Node(Ply(ringPlacementMoves[i])));
		Node *& tempNode = node->neighbors.back();
		extendTreeMove(tempNode,currAlpha,currBeta,currDepth);
		compare(tempNode->value,currAlpha,currBeta,turn);
		change(node->value,tempNode->value,turn);
		if(currAlpha>currBeta) {
			return;
		}
	}
	node->explored = 1;
}
/*The following is the driver function. Each node calls the driver function for expanding its nodes. It decides
whether to remove rows or move rings. It updates the depth and turn values as well*/

// Check the logic of this function (important)

void extendTree(Node *&node,double alpha,double beta,int currDepth,bool removeRows) { // Driver function
	if(depth == currDepth) {
		vector<RowInfo> rows;
		executePly(node->plyTaken,rows);
		node->value = evaluateGameBoard(); // Features filled
		reversePly(node->plyTaken);
		return;
	}
	if(player_id == 0 and currDepth <=10) {
		previousTurn = turn;
		turn = (turn+1)%2;
		extendTreePlaceRing(node,alpha,beta,currDepth+1);
	} else if(node->myRows.size() == 0 and node->otherRows.size() == 0) {
		if(removeRows == 0) { // No rows removed in last turn
			previousTurn = turn;
			turn = (turn+1)%2;
			extendTreeMove(node,alpha,beta,currDepth+1);
		} else { // Rows Removed in last turn
			if(previousTurn == turn) { // Move and removeRows
				turn = (turn+1)%2;
				extendTreeMove(node,alpha,beta,currDepth+1);
			} else { // Rows removed by other player and rows removed by this player
				previousTurn = turn;
				extendTreeMove(node,alpha,beta,currDepth+1);
			}
		}
	} else if(node->myRows.size()!=0) {
		if(removeRows == 1) {
			previousTurn == turn;
			turn = (turn+1)%2;
			extendTreeRemoval(node,alpha,beta,currDepth+1);
		} else {
			previousTurn = turn;
			extendTreeRemoval(node,alpha,beta,currDepth+1);
		}
	} else if(node->otherRows.size()!=0) {
		if(removeRows == 1) {
			previousTurn == turn;
			turn = (turn+1)%2;
			extendTreeRemoval(node,alpha,beta,currDepth+1);
		} else {
			previousTurn = turn;
			extendTreeRemoval(node,alpha,beta,currDepth+1);
		}
	}
}
void createGameTree(int depthToSearch) { // Create GameTree
	Player player;
	depth = depthToSearch;
	turn = 0;
	if(player_id == 0) {
		player = player1;
	} else player = player2;
	double alpha = inf;
	double beta = minf;
	GameTree->explored = 1;
	extendTreeMove(GameTree,alpha,beta,0);
	return;
}
void plyAhead(Ply plyTaken) { // Performs one move. Removes irrelevant parts of the GameTree
	int i;
	for(i=0; i<GameTree->neighbors.size(); i++) {
		if(GameTree->neighbors[i]->plyTaken == plyTaken) {
			break;
		}
		delete(GameTree->neighbors[i]);
	}
	for(int k=i+1; k<GameTree->neighbors.size(); k++) {
		delete(GameTree->neighbors[k]);

	}
	GameTree = GameTree->neighbors[i];
	createGameTree(depth);
}
int main() {
	cin >> player_id >> board_size >> game_timer;
	board = new int*[(board_size-1)/2];
	for(int hex = 1; hex<=(board_size-1)/2; hex++) {
		board[hex] = new int[6*hex];
	}
	board[0] = new int[1];
	return 0;
}
