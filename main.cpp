//
//  main.cpp
//  csv_tool
//
//  Created by Michael O'Meara on 6/25/16.
//  Copyright (c) 2016 Michael O'Meara. All rights reserved.
//

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>
#include <cstddef>        // std::size_t
#include <math.h>
#include <vector>
#include <map>

using namespace std;

// Define data types for holding CSV data
typedef double field_t;
typedef vector<field_t> record_t;
typedef vector<record_t> data_t;

#include "utils.h"

// Show usage details
static void show_usage(string name)
{
    cerr << "Usage: " << name << " --csv=FILENAME.csv --cols=col1,col2 --operator=plus --rows=10"
    << endl << "or " << endl
    << name << " --tables=FILENAME.csv,FILENAME2.csv --join=inner --on=col0,col0" << endl;
}


int main(int argc, const char * argv[]) {
    
    // Check if we have enough command line arguements
    if (argc < 4) {
        show_usage(argv[0]);
        return 1;
    }
    
    // Declare a bunch of variables to hold command line arguments
    string arg;                     // temporary placeholder for argv values
    string tmp;
    string csv;                     // CSV file name
    vector<int> cols;               // integer values
    vector<string> cols_str;        // string values
    string optr;                    // store operator value
    vector<string> tables;          // CSV file names from --tables=
    vector<int> index;              // --on= table index values
    string join;                    // --join= value
    size_t rows = 0;                // number of rows to display
    
    // Read the command line arguements and store them in the variables defined above
    for (int i = 1; i < argc; ++i) {
        arg = argv[i];
        transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
        if ((arg == "-h") || (arg == "--help")) {
            show_usage(argv[0]);
            return 0;
        } else if (arg.substr(0,5) == "--csv") {
            csv = arg.substr(6);
        } else if (arg.substr(0,8) == "--tables") {
            tmp = arg.substr(9);
            tables = split(tmp, ',');
        } else if (arg.substr(0,4) == "--on") {
            tmp = arg.substr(5);
            vector<string> tmp_index = split(tmp, ',');
            for (int j = 0; j < tmp_index.size(); j++)
            {
                const string s = tmp_index[j].substr(3);
                index.push_back(stoi(s));
            }
        } else if (arg.substr(0,6) == "--join") {
            join = arg.substr(7);
            transform(join.begin(), join.end(), join.begin(), ::tolower);
        } else if (arg.substr(0,6) == "--cols") {
            tmp = arg.substr(7);
            cols_str = split(tmp, ',');
            for (int j = 0; j < cols_str.size(); j++)
            {
                size_t npos = cols_str[j].find_first_of("1234567890");
                string s = cols_str[j].substr(npos);
                cols.push_back(stoi(s));
            }
        } else if (arg.substr(0,10) == "--operator") {
            if (arg.size() > 11) {
                optr = arg.substr(11);
                transform(optr.begin(), optr.end(), optr.begin(), ::tolower);
            } else {
                show_usage(argv[0]);
                return 0;
            }
        } else if (arg.substr(0,6) == "--rows") {
            if (arg.size() > 7)
                rows = stoi(arg.substr(7));
            else {
                cerr << "\n--rows argument was empty, showing all rows." << endl;
            }
        } else {
            show_usage(argv[0]);
            return 0;
        }
    }
    
    
    data_t result, data1, data2;
    
    // if string, csv, is empty we are in the mode to join two tables from different CSV files
    if (csv.empty() && tables.size() == 2) {
        
        // Here is the file containing the data. Read it into data1.
        ifstream infile( tables[0] );
        infile >> data1;
        
        // Something went wrong.
        if (!infile.eof())
        {
            cout << "A problem has occured!\n";
            return 1;
        }
        
        infile.close();

        // Here is the file containing the data for table2. Read it into data2.
        ifstream infile2( tables[1] );
        infile2 >> data2;
        
        // Something went wrong.
        if (!infile2.eof())
        {
            cout << "A problem has occured!\n";
            return 1;
        }
        
        infile2.close();
        
        cout << tables[0] << " contains " << data1.size() << " records." << endl;
        cout << tables[1] << " contains " << data2.size() << " records." << endl;
        cout << endl;
        
        if (rows == 0) rows = data1.size();
        
        if (join.compare("inner") == 0)
            result = inner_join_tables(data1, data2, index, rows);
        else if (join.compare("outer") == 0)
            result = outer_join_tables(data1, data2, index, rows);
        else {
            cout << "--join argument needs value inner or outer" << endl;
            show_usage(argv[0]);
        }
        
        cout << endl;
        
        vector<string> stat_labels = {"min", "max", "median", "average"};
        
        vector<field_t> stats;
        for (int k = 0; k < result[0].size(); k++)
        {
            stats = stats_on_column(result, rows, k);
            
            cout << "  Stats for Column" << k << ":" << endl;
            for (int i = 0; i < stats.size(); i++)
                cout << "\t" << setw(10) << right << stat_labels[i] << ": " << stats[i];
            
            cout << endl;
        }

        
    } else {
        
        // Here is the file containing the data. Read it into data1.
        ifstream infile( csv );
        infile >> data1;
        
        // Something went wrong.
        if (!infile.eof())
        {
            cout << "A problem has occured!\n";
            return 1;
        }
        
        infile.close();
        
        if (rows == 0) rows = data1.size();
        
        // Artificially add all columns if none were specified
        if (cols.empty())
            for (int c = 0; c < data1[0].size(); c++) {
                stringstream sstm;
                sstm << "col" << c;
                cols_str.push_back(sstm.str());
                cols.push_back(c);
            }
        
        // Information about the file.
        cout << "\nCSV file contains " << data1.size() << " records." << endl << endl;
        //cout << "Each record contains " << data1[0].size() << " columns." << endl;
        
        cols.push_back((int)(data1[0].size()+1));
        
        
        if (optr.compare("plus") == 0 || optr.compare("add") == 0 || optr.compare("sum") == 0) {
            process_columns(data1, rows, cols, sum);
        } else if (optr.compare("minus") == 0 || optr.compare("subtract") == 0 || optr.compare("difference") == 0) {
            process_columns(data1, rows, cols, subtract);
        } else if (optr.compare("times") == 0 || optr.compare("multiply") == 0 || optr.compare("product") == 0) {
            process_columns(data1, rows, cols, times);
        } else if (optr.compare("divide") == 0) {
            process_columns(data1, rows, cols, divide);
        } else {
            cerr << "No valid operator specified." << endl;
            return 1;
        }
        
        for (int i = 0; i < cols_str.size(); i++)
            cout << "\t" << setw(10) << left << cols_str[i];
        
        cout << "\t" << setw(10) << left << "Result" << endl;
        cout << "     " << setfill('-') << setw((int)(15*(cols.size()))) << " " << endl;
        
        process_columns(data1, rows, cols, print);
        
        cout << endl;
        
        vector<string> stat_labels = {"min", "max", "median", "average"};
        
        vector<field_t> stats;
        for (int k = 0; k < cols.size()-1; k++)
        {
            stats = stats_on_column(data1, rows, cols[k]);
        
            cout << "  Stats for " << cols_str[k] << ":" << endl;
            for (int i = 0; i < stats.size(); i++)
                cout << "\t" << right << stat_labels[i] << ": " << stats[i];
        
            cout << endl;
        }
    }
    
    cout << endl;
    
    
    return 0;
}
