#ifndef P_STREAM_H
#define P_STREAM_H

#include <string>
#include <iostream>
#include <vector>
#include <fstream>

/* The manipulator class for manipulator with one parameter */
namespace pst {
   
    //stream status
    constexpr int  good = 0x0000;
    constexpr int  fail = 0x0001;

    //margins ninimum values
    constexpr int MARGINMIN = 3; //millimetres

    template <typename T> class pstream_manip;

	class pstream {

    public:
        pstream();
        pstream(const std::string& which_printer);
        ~pstream();

        operator bool() {
            return !stream_status_;
        }

        
        void set_title(const std::string& new_title)   { title_ = new_title; }
        void set_page_numbers(const bool b_val)        { page_numbers_ = b_val;}
        void set_tab_size(const int new_tab_size)      { tab_size_ = new_tab_size;}
        
        /* The following are member functions NOT manipulators */        
        void setLeftMargin(int newMargin)   {left_margin_ = newMargin <= MARGINMIN? MARGINMIN:newMargin; }
        void setTopMargin(int newMargin)    {top_margin_ = newMargin <= MARGINMIN? MARGINMIN:newMargin; };
        void setRightMargin(int newMargin)   {right_margin_ = newMargin <= MARGINMIN? MARGINMIN:newMargin; };
        void setBottomMargin(int newMargin)   {bottom_margin_ = newMargin <= MARGINMIN? MARGINMIN:newMargin; };

        
        
        //Insertion Overloads
        //Add a string
        pstream& operator << (std::string name);
        //Add a char buffer
        pstream& operator << (char* name);
        //Add a file
        pstream& operator << (std::ifstream & in_file);

        //other insertion overlaods
        pstream& operator << (pstream & (*ptr)(pstream&)); //manipulator with no parameters
        /*!
        \brief Insertion overload for with manipulators with one parameter
        Note this is a template function so we put the definition here - much easier
        */
        template <typename T>
        pstream& operator << (pstream_manip<T> manip)
        {
            return manip(*this);
        }

        /* Useful static functions */
        static std::vector<std::string> get_printer_names();        

    private:
        std::string defaultPrinter;
        int stream_status_;
        std::vector<std::string> lines;
        int left_margin_;
        int right_margin_;
        int top_margin_;
        int bottom_margin_;
        std::string title_;
        bool page_numbers_;
        int tab_size_;

        /* copy constructor - private and no body */
        pstream(const pstream& other);
        
        /* manipulators
         * These are mostly friends of pstream.
         * They are in the pst namespace
         */

        /* Manipulators taking no arguments */
        friend pstream& flushp (pstream& p);

		/* manipulators taking one or more arguments 
         * These are structure based
         */        
        template <typename T>
        friend pstream_manip<T> set_left_margin (T margin);

        template <typename T>
        friend pstream_manip<T> set_right_margin (T margin);

        template <typename T>
        friend pstream_manip<T> set_top_margin (T margin);

        template <typename T>
        friend pstream_manip<T> setBottomMargin (T margin);

        /* private functions for the manipulator(s) with arguments. These are static */
        static pstream& set_margin_l(pstream & p, int n);
        static pstream& set_margin_r(pstream & p, int n);
        static pstream& set_margin_t(pstream & p, int n);
        static pstream& set_margin_b(pstream & p, int n);

    }; // class pstream


    /* The manipulator class for manipulator with one parameter */
    template <typename T>
    class pstream_manip {
    public:
        pstream_manip(pstream& (*fp)(pstream&, T val), T arg) : pf(fp), arg_value_(arg) { };
        pstream& operator() (pstream & ps) { 
            return (*pf)(ps, arg_value_);
        }

    private:
        pstream& (*pf)(pstream&, T);
        T arg_value_;

    }; // class pstream

    /* all manipulators are in the pst namespace */
    pstream& flushp (pstream& p);

    
    template <typename T> 
    pstream_manip<T> set_left_margin (T margin) {
        return pstream_manip<T>(pstream::set_margin_l, margin);
    }

    template <typename T> 
    pstream_manip<T> set_right_margin (T margin) {
        return pstream_manip<T>(pstream::set_margin_r, margin);
    }

    template <typename T> 
    pstream_manip<T> set_top_margin (T margin) {
        return pstream_manip<T>(pstream::set_margin_t, margin);
    }

    template <typename T> 
    pstream_manip<T> setBottomMargin (T margin) {
        return pstream_manip<T>(pstream::set_margin_b, margin);
    }
}



#endif 