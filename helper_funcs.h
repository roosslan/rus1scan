#include "stdafx.h"

#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

static constexpr unsigned char permanent_config = 0;
static constexpr unsigned char temporary_config = 1;

enum class rus1_action {
	print,
	start,
	nothing,
	save,
	save_by_date
};

struct act_event {
	std::string device_name;
	std::string device_guid;
	std::string btn_event_name;
	std::string event_guid;
	int index_of_device;
	rus1_action action;
	std::string param;
};

std::string bstr_to_u32(BSTR source);
std::string bstr_to_std_string(BSTR bstr);

void bmp_to_pdf(const std::string& image_name, const std::string& dir_n_pdfname);
bool load_printers(CComboBox* combo_box_printers);
bool fill_cb_events(CComboBox* combo_box_devices, CComboBox* combo_box_events, std::vector<act_event>* act_events);
void print_wia_img(const std::string& file_name, const std::string& printer_name);
bool save_action_to_file(const std::string& action_to_save);

#endif