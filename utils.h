//
//  utils.h
//  csv_tool
//
//  Created by Michael O'Meara on 6/25/16.
//  Copyright (c) 2016 Michael O'Meara. All rights reserved.
//

#ifndef csv_tool_utils_h
#define csv_tool_utils_h

/*
 *  Read each line into a record vector by overloading the >> operator for istream
 */

istream& operator >> ( istream& ins, record_t& record )
{
    // make sure that the returned record contains only the stuff we read now
    record.clear();
    
    // read the entire line into a string (a CSV record is terminated by a newline)
    string line;
    getline( ins, line );
    
    size_t hpos = line.find_first_of("1234567890");
    // Skip the first line if it doesn't contain any digits
    if (hpos > line.size())
    {
        getline( ins, line );
    }
    
    // use stringstream to the line into fields
    stringstream ss( line );
    string field;
    string field2;
    while ( getline( ss, field, ',' ) )
    {
        // remove dashes in case field is a date string
        remove_copy(field.begin(), field.end(), back_inserter(field2), '-');
        // for each field, convert it to a field_t
        stringstream fs( field2 );
        field_t f = 0.0;  // (default value is 0.0)
        fs >> f;
        
        // add the new field to the end of the record
        record.push_back( f );
        
        field.clear();
        field2.clear();
        fs.clear();
        
    }
        
    // return the stream as required for this kind of input overload function.
    return ins;
}

/*
 *  Overload the stream input operator to read a list of CSV records using the overloaded
 *  istream >> operator to insert the line into a record.
 */

istream& operator >> ( istream& ins, data_t& data )
{
    // make sure that the returned data only contains the CSV data we read here
    data.clear();
    
    // For every record we can read from the file, append it to our resulting data
    record_t record;
    while (ins >> record)
    {
        data.push_back( record );
    }
    
    // Return the stream, required for input stream overload.
    return ins;
}

/*
 *  Split string by delimiter using existing vector
 */

vector<string> &split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/*
 *  Split string by delimiter returning result as a new vector
 */

vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

/*
 *  Perform column operations and insert the result into the last column of data
 *  - input arguments multidimensional vector data,
 *      columns to operate on and the operation to perform
 */

void process_columns(vector<vector<field_t>>& data, const size_t rows, const vector<int>& column,
                     function<field_t(record_t,const vector<int>&)> f)
{
    
    float val = 0.0f;
    int rec_size = column[column.size()-1]+1;
    
    for (int i = 0; i < rows; i++)
    {
        val = f(data[i], column);
        data[i].resize(rec_size);
        data[i][rec_size-1] = val;
    }
    
}

// Sum two columns specified by --cols argument
field_t sum(record_t rec, const vector<int>& column)
{
    return rec[column[0]] + rec[column[1]];
}

// Subtract two columns specified by --cols argument
field_t subtract(record_t rec, const vector<int>& column)
{
    return rec[column[0]] - rec[column[1]];
}

// Multiply two columns specified by --cols argument
field_t times(record_t rec, const vector<int>& column)
{
    return rec[column[0]] * rec[column[1]];
}

// Divide two columns specified by --cols argument
field_t divide(record_t rec, const vector<int>& column)
{
    return rec[column[0]] / rec[column[1]];
}

field_t print(record_t rec, const vector<int>& column)
{
    for (int field : column)
        cout << "\t" << setfill(' ') << setw(10) << left << setprecision(7) << rec[field];
    
    cout << endl;
    
    return 0.0f;
}


/*
 *  Return stats on a select column of the data
 *      - result vector contains [min, max, median, average]
 */

vector<field_t> stats_on_column(vector<vector<field_t>>& data, const size_t rows, size_t column)
{
    vector<field_t> array, result;
    float tmp;
    float average = 0.0f;
    size_t i, sz;
    
    i = 0;
    array.clear();
    for (int i = 0; i < data.size(); i++) {
        if (i >= rows)
            break;
        tmp = data[i][column];
        array.push_back(tmp);
        average += tmp;
    }
    
    sz = array.size();
    average = average / sz;                     // calculate average
    
    sort(array.begin(), array.end());

    result.push_back(array[0]);                 // min value
    result.push_back(array[sz-1]);              // max value
    
    // Check if size of array is even or odd
    if (sz % 2 == 0) {
        size_t p1,p2;
        p1 = sz / 2 - 1;
        p2 = p1 + 1;
        result.push_back((array[p1]+array[p2])/2);  // median when even number of records
        // 0 1 2 3
    } else {
        size_t p = 0;
        if (sz != 1)
            p = (int)ceil((float)sz/2)-1;
        result.push_back(array[p]);      // median when odd number of records
    }
    result.push_back(average);                      // insert average into result
    
    return result;
}

/*
 *  Do an inner join on the two tables using indices as the matching key value between the datasets
 *      indices contains two values, one value representing the key for each column. indices[0] is for table1 and
 *      indices[1] is for table2
 */

vector<vector<field_t>> inner_join_tables(const vector<vector<field_t>>& table1, const vector<vector<field_t>>& table2,
                       vector<int> indices, const size_t rows)
{
    map<long, vector<field_t>> map1;
    map<long, vector<field_t>> map2;
    map<long, vector<field_t>>::iterator m_it;
    
    vector<vector<field_t>> data1;
    vector<vector<field_t>> data2;
    int index1, index2;
    field_t key;
    
    // Pick the smallest table to minimize the number of insertions
    if (table1.size() < table2.size()) {
        data1 = table1;
        data2 = table2;
        index1 = indices[0];
        index2 = indices[1];
    } else {
        data1 = table2;
        data2 = table1;
        index1 = indices[1];
        index2 = indices[0];
    }
    
    // Insert smallest table into hash table
    for (record_t rec : data1)
    {
        key = rec[index1];
        if (key)
            for (record_t::iterator it = rec.begin(); it != rec.end(); ++it)
                if (*it != key)
                    map1[key].push_back(*it);
        
    }
    
    // For each record in the other table do the keys exist in both, if so add all fields to map2
    for (record_t rec : data2)
    {
        key = rec[index2];
        if (key)
        {
            m_it = map1.find(key);
            if (m_it != map1.end()) {
                record_t rec1 = m_it->second;
                for (record_t::iterator it1 = rec1.begin(); it1 != rec1.end(); ++it1) {
                    if (*it1 != key)
                        map2[key].push_back(*it1);
                }
                for (record_t::iterator it2 = rec.begin(); it2 != rec.end(); ++it2) {
                    if (*it2 != key)
                        map2[key].push_back(*it2);
                }
            }
        }
    }
    
    // Force key to have a value
    key = map2.begin()->first;
    
    // Print the result
    vector<int> columns;
    for (int i = 0; i < map2[key].size(); i++)
        columns.push_back(i);
    
    data1.clear();
    size_t iters = 0;
    for (map<long, vector<field_t>>::iterator it = map2.begin(); it != map2.end(); ++it)
    {
        if (iters >= rows)
            break;

        data1.push_back(it->second);
        print(it->second, columns);
        ++iters;
    }
    
    return data1;
}

/*
 *  Do an outer join on the two tables using indices as the matching key value between the datasets.  
 *      indices contains two values, one value representing the key for each column. indices[0] is for table1 and
 *      indices[1] is for table2
 */

vector<vector<field_t>> outer_join_tables(const vector<vector<field_t>>& table1, const vector<vector<field_t>>& table2,
                       vector<int> indices, const size_t rows)
{
    map<long, vector<field_t>> map1;
    map<long, vector<field_t>>::iterator m_it;
    vector<vector<field_t>> data;
    
    size_t i, table1_width, table2_width;
    field_t key;
    
    table1_width = table1[0].size();
    table2_width = table1[1].size();
    
    // Insert table1 into hash table
    for (record_t rec : table1)
    {
        key = rec[indices[0]];
        if (key) {
            for (record_t::iterator it = rec.begin(); it != rec.end(); ++it)
                if (*it != key)
                    map1[key].push_back(*it);
            for (i = 0; i < table2_width-1; i++)
                map1[key].push_back(0.0);
        }
    }
    
    //
    for (record_t rec : table2)
    {
        key = rec[indices[1]];
        if (key)
        {
            m_it = map1.find(key);
            if (m_it != map1.end()) {
                for (i = 0; i < table2_width; i++) {
                    if (rec[i] != key)
                        map1[key][i+table1_width] = rec[i];
                    
                }
            } else {
                for (i = 0; i < table1_width-1; i++)
                        map1[key].push_back(0.0);
                
                for (i = 0; i < table2_width; i++) {
                    if (rec[i] != key)
                        map1[key].push_back(rec[i]);
    
                }
                
            }
            rec.clear();
            
        }
    }
    
    // Force key to have a value
    key = map1.begin()->first;
    
    // Print the result
    vector<int> columns;
    for (int i = 0; i < map1[key].size(); i++)
        columns.push_back(i);
    
    size_t iters = 0;
    for (map<long, vector<field_t>>::iterator it = map1.begin(); it != map1.end(); ++it)
    {
        if (iters >= rows)
            break;
        
        data.push_back(it->second);
        print(it->second, columns);
        
        ++iters;
    }
    
    return data;
}


#endif
