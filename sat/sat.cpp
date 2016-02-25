#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>

using namespace std;

#define LITS_PER_CLAUSE 3
#define UNDEF -1
#define TRUE 1
#define FALSE 0

typedef vector<int> Clause;

uint numVars;
uint numClauses;
vector<Clause> clauses; //{l12, l59, l3, ...} all positive

vector<int> model;      //{l0:FALSE, l1:FALSE, l2:TRUE, l3:UNDEF, l4:TRUE, l5:UNDEF, l6:FALSE, ...}
vector<int> modelStack; //{0,3,54,6,1, 0,9,7,65,19

//For the lit i, it contains all the clauses where it appears
vector< vector<Clause> > litToClausesWhereIsPositive;
vector< vector<Clause> > litToClausesWhereIsNegative;


//For the lit i, it contains the index in which is contained into the
//litsOrderedByFrequencyDesc vector
map<int,int> litToLitIndexInFreqVector;
//<Literal, Frequency>
vector< pair<int,int> > litsOrderedByFrequencyDesc;


int propagations = 0;
int decisions = 0;

uint indexOfNextLitToPropagate;
uint decisionLevel;

void printClause(const Clause &c)
{
    cout << "clause:(";
    for (uint i = 0; i < c.size()-1; ++i)
    {
        cout << c[i] << " || ";
    }
    cout << c[c.size()-1] << ")" << endl;
}

bool sortByFreq(const pair<int,int>& left, const pair<int,int>& right)
{
    return left.second > right.second;
}

void readClauses()
{
  // Skip comments
  char c = cin.get();
  while (c == 'c')
  {
    while (c != '\n') c = cin.get();
    c = cin.get();
  }  

  // Read "cnf numVars numClauses"
  string aux;
  cin >> aux >> numVars >> numClauses;
  litToClausesWhereIsPositive.resize(numVars);
  litToClausesWhereIsNegative.resize(numVars);

  litsOrderedByFrequencyDesc.resize(numVars);
  for(uint i = 0; i < numVars; ++i)
  {
      litsOrderedByFrequencyDesc[i].first = (i+1); //lit
      litsOrderedByFrequencyDesc[i].second = 0;    //freq=0
  }

  clauses.resize(numClauses);

  // Read clauses
  for (uint i = 0; i < numClauses; ++i)
  {
    int lit;
    int lits[LITS_PER_CLAUSE]; int j = 0;
    while (cin >> lit && lit != 0)
    {
        clauses[i].push_back(lit);
        lits[j++] = lit;
    }

    for(uint k = 0; k < LITS_PER_CLAUSE; ++k)
    {
        lit = lits[k];

        litsOrderedByFrequencyDesc[abs(lit)-1].second++; //Increase frequency

        if(lit > 0) litToClausesWhereIsPositive[abs(lit)-1].push_back(clauses[i]);
        else        litToClausesWhereIsNegative[abs(lit)-1].push_back(clauses[i]);
    }
  }

  std::sort(litsOrderedByFrequencyDesc.begin(),
            litsOrderedByFrequencyDesc.end(),
            sortByFreq);

  /*
  for(uint i = 0; i < litsOrderedByFrequencyDesc.size(); ++i)
  {
      int lit = litsOrderedByFrequencyDesc[i].first;
      //litToLitIndexInFreqVector[lit] = i; //add the lit and the index to the map
  }
  */
}

int currentValueInModel(int lit)
{
  if (lit >= 0)
  {
      return model[lit];
  }
  else
  {
    if (model[-lit] == UNDEF) return UNDEF;
    else return 1 - model[-lit];
  }
}


void setLiteralToTrue(int lit)
{
  modelStack.push_back(lit);
  if (lit > 0)
  {
      model[lit] = TRUE;
  }
  else
  {
      model[-lit] = FALSE;
  }
}


bool propagateGivesConflict()
{
    while ( indexOfNextLitToPropagate < modelStack.size() )
    {
        //Propagate last decision
        int lit = modelStack[indexOfNextLitToPropagate];
        ++indexOfNextLitToPropagate;
        ++propagations;

        const vector<Clause> &clausesToCheckForConflict = (currentValueInModel(abs(lit)) == TRUE) ?
                                                           litToClausesWhereIsNegative[abs(lit)-1] :
                                                           litToClausesWhereIsPositive[abs(lit)-1];

        //The decision is to make lit=TRUE/FALSE, so lets check for conflict in
        //the clauses where it is apporting nothing (where is negative/positive)
        //as the other ones are surely giving TRUE because of this decission
        //so there there wont be any conflict
        const uint S = clausesToCheckForConflict.size();
        for(uint i = 0; i < S; ++i)
        {
            int numUndefs = 0;
            int lastLitUndef = 0;
            bool someLitTrue = false;
            for(uint j = 0; !someLitTrue && j < LITS_PER_CLAUSE; ++j)
            {
                const int jval = currentValueInModel(clausesToCheckForConflict[i][j]);
                if(jval == TRUE)
                {
                    someLitTrue = true;
                }
                else if (jval == UNDEF)
                {
                    ++numUndefs;
                    lastLitUndef = clausesToCheckForConflict[i][j];
                }
            }

            if (!someLitTrue) //for each clause
            {
                if(numUndefs == 0)
                {
                   // cout << "CONFLICT! in clause: " << &(clausesToCheckForConflict[i]) << endl;
                    return true; // conflict! all lits false
                }
                if(numUndefs == 1)
                {
                    setLiteralToTrue(lastLitUndef);
                }
            }
        }

    }

    return false;
}


void backtrack()
{
  uint i = modelStack.size() -1;
  int lit = 0;
  while (modelStack[i] != 0) // 0 is the DL mark
  {
    lit = modelStack[i];
    model[abs(lit)] = UNDEF;
    modelStack.pop_back();
    --i;
  }

  // at this point, lit is the last decision
  modelStack.pop_back(); // remove the DL mark
  --decisionLevel;
  indexOfNextLitToPropagate = modelStack.size();
  setLiteralToTrue(-lit);  // reverse last decision
}


// Heuristic for finding the next decision literal:
int getNextDecisionLiteral()
{
   // cerr << "------------" << endl;
  //*
  for(uint i = 0; i < numVars; ++i)
  {
     // cerr << litsOrderedByFrequencyDesc[i].first << ":" <<  litsOrderedByFrequencyDesc[i].second << endl;
      if(currentValueInModel(litsOrderedByFrequencyDesc[i].first) == UNDEF)
          return litsOrderedByFrequencyDesc[i].first;
  }
  //*/

  /*
  for (uint i = 1; i <= numVars; ++i)  // stupid heuristic:
  {
    if (model[i] == UNDEF)
    {
        return i;
    } // returns first UNDEF var, positively
  }
  //*/

  return 0; // reurns 0 when all literals are defined
}

void checkmodel()
{
  for (uint i = 0; i < numClauses; ++i)
  {
    bool someTrue = false;
    uint j;
    for (j = 0; not someTrue and j < clauses[i].size(); ++j)
    {
      someTrue = (currentValueInModel(clauses[i][j]) == TRUE);
    }
    if (not someTrue)
    {
      cout << "Error in model, clause is not satisfied:";
      printClause(clauses[j]);
      cout << endl;
      exit(1);
    }
  }  
}

void printTimePropsAndDecisions(clock_t start)
{
    double timeTaken = ((double)(clock() - start) / CLOCKS_PER_SEC);
    cout  <<  "\tTime: " << timeTaken << "\ts" <<
              "\tDecisions: " << decisions << "   "
              "\tP/s: " << ((double) propagations) / timeTaken << endl;
}

int main()
{
  clock_t start = clock();

  readClauses(); // reads numVars, numClauses and clauses
  model.resize(numVars+1,UNDEF);
  indexOfNextLitToPropagate = 0;  
  decisionLevel = 0;

  // Take care of initial unit clauses, if any
  for (uint i = 0; i < numClauses; ++i)
    if (clauses[i].size() == 1)
    {
      int lit = clauses[i][0];
      int val = currentValueInModel(lit);
      if (val == FALSE) {cout << "UNSATISFIABLE" << endl; return 10;}
      else if (val == UNDEF) setLiteralToTrue(lit);
    }
  
  // DPLL algorithm
  while (true)
  {
    while ( propagateGivesConflict() )
    {
      if ( decisionLevel == 0)
      {
          cout << "UNSATISFIABLE"; printTimePropsAndDecisions(start);
          return 10;
      }

      backtrack();
    }

    int decisionLit = getNextDecisionLiteral();
    if (decisionLit == 0)
    {
        checkmodel();
        cout << "SATISFIABLE"; printTimePropsAndDecisions(start);
        return 20;
    }

    // start new decision level:
    ++decisions;
    ++decisionLevel;
    ++indexOfNextLitToPropagate;
    modelStack.push_back(0);       // push mark indicating new DL
    setLiteralToTrue(decisionLit); // now push decisionLit on top of the mark
  }
}  
