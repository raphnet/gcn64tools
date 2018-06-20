/*	gc_n64_usb : Gamecube or N64 controller to USB adapter firmware
	Copyright (C) 2007-2015  Raphael Assenat <raph@raphnet.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "requests.h"
#include "mempak.h"
#include "gcn64ctl_gui.h"
#include "mempak_gcn64usb.h"
#include "gcn64lib.h"
#include "uiio_gtk.h"
#include "mempak_fill.h"

#ifdef WINDOWS
#include <windows.h>
#endif

/* Set sensitivity to FALSE on items that depend on an adapter being selected */
static void desensitize_adapter_widgets(struct application *app)
{
	GtkWidget *widgets[] = {
		GET_ELEMENT(GtkWidget, adapterDetails),
		GET_ELEMENT(GtkWidget, menuitem_suspend_polling),
		GET_ELEMENT(GtkWidget, menuitem_resume_polling),
		GET_ELEMENT(GtkWidget, menu_manage_gc2n64),
		GET_ELEMENT(GtkWidget, menu_manage_x2gcwii),
		NULL
	};
	int i;

	for (i=0; widgets[i]; i++) {
		gtk_widget_set_sensitive(widgets[i], FALSE);
	}
}

static void setsensitive_n64_adapter_widgets(struct application *app, int sensitive)
{
	GtkWidget *widgets[] = {
		GET_ELEMENT(GtkWidget, menu_manage_gc2n64),
		GET_ELEMENT(GtkWidget, btn_read_mempak),
		GET_ELEMENT(GtkWidget, btn_write_mempak),
		GET_ELEMENT(GtkWidget, btn_erase_mempak),
		GET_ELEMENT(GtkWidget, menuitem_display_cart_info),
		GET_ELEMENT(GtkWidget, menuitem_read_cart_rom),
		GET_ELEMENT(GtkWidget, menuitem_read_cart_ram),
		GET_ELEMENT(GtkWidget, menuitem_write_cart_ram),
		NULL
	};
	int i;

	for (i=0; widgets[i]; i++) {
		gtk_widget_set_sensitive(widgets[i], sensitive);
	}
}

static void setsensitive_gc_adapter_widgets(struct application *app, int sensitive)
{
	GtkWidget *widgets[] = {
		GET_ELEMENT(GtkWidget, menu_manage_x2gcwii),
		NULL
	};
	int i;

	for (i=0; widgets[i]; i++) {
		gtk_widget_set_sensitive(widgets[i], sensitive);
	}
}

void deselect_adapter(struct application *app)
{
	GET_UI_ELEMENT(GtkComboBox, cb_adapter_list);

	printf("deselect adapter\n");

	if (app->current_adapter_handle) {
		rnt_closeDevice(app->current_adapter_handle);
		app->current_adapter_handle = NULL;
		desensitize_adapter_widgets(app);
	}

	gtk_combo_box_set_active_iter(cb_adapter_list, NULL);
}

void infoPopup(struct application *app, const char *message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(app->mainwindow,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_INFO,
									GTK_BUTTONS_CLOSE,
									message);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


void errorPopup(struct application *app, const char *message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(app->mainwindow,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR,
									GTK_BUTTONS_CLOSE,
									message);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static gboolean periodic_updater(gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkLabel, label_controller_type);
	GET_UI_ELEMENT(GtkButton, btn_rumble_test);

	if (app->current_adapter_handle && !app->inhibit_periodic_updates) {
		app->controller_type = rnt_getControllerType(app->current_adapter_handle, 0);
		gtk_label_set_text(label_controller_type, rnt_controllerName(app->controller_type));

		setsensitive_n64_adapter_widgets(app, FALSE);
		setsensitive_gc_adapter_widgets(app, FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(btn_rumble_test), FALSE);

		switch (app->controller_type)
		{
			case CTL_TYPE_N64:
				gtk_widget_set_sensitive(GTK_WIDGET(btn_rumble_test), TRUE);
				setsensitive_n64_adapter_widgets(app, TRUE);
				break;

			case CTL_TYPE_GC:
				gtk_widget_set_sensitive(GTK_WIDGET(btn_rumble_test), TRUE);
				setsensitive_gc_adapter_widgets(app, TRUE);
				break;

			default:
			case CTL_TYPE_NONE:
				break;
		}

	}
	return TRUE;
}

void syncGuiToCurrentAdapter(struct application *app)
{
	unsigned char buf[32];
	int n;
	struct {
		unsigned char cfg_param;
		GtkWidget *widget;
		uint32_t feature_bit; // If zero, always visible and available.
		gboolean hide_when_unavailable;
	} configurable_bits[] = {
//		{ 0, GET_ELEMENT(GtkWidget, btn_suspend_polling), RNTF_SUSPEND_POLLING, FALSE },
//		{ 0, GET_ELEMENT(GtkWidget, btn_resume_polling), RNTF_SUSPEND_POLLING, FALSE },
		{ 0, GET_ELEMENT(GtkWidget, menuitem_suspend_polling), RNTF_FW_UPDATE, FALSE },
		{ 0, GET_ELEMENT(GtkWidget, menuitem_resume_polling), RNTF_FW_UPDATE, FALSE },
		{ 0, GET_ELEMENT(GtkWidget, btn_update_firmware), RNTF_FW_UPDATE, FALSE },
		{ 0, GET_ELEMENT(GtkWidget, box_poll_interval), RNTF_POLL_RATE, TRUE },
		{ 0, GET_ELEMENT(GtkWidget, lbl_controller_type), RNTF_CONTROLLER_TYPE, TRUE },
		{ 0, GET_ELEMENT(GtkWidget, label_controller_type), RNTF_CONTROLLER_TYPE, TRUE },
		{ 0, GET_ELEMENT(GtkWidget, frame_adapter_mode), RNTF_ADAPTER_MODE, TRUE },

		{ CFG_PARAM_FULL_SLIDERS, GET_ELEMENT(GtkWidget, chkbtn_gc_full_sliders), RNTF_GC_FULL_SLIDERS, TRUE },
		{ CFG_PARAM_INVERT_TRIG, GET_ELEMENT(GtkWidget, chkbtn_gc_invert_trig), RNTF_GC_INVERT_TRIG, TRUE },
		{ CFG_PARAM_TRIGGERS_AS_BUTTONS, GET_ELEMENT(GtkWidget, chkbtn_sliders_as_buttons), RNTF_TRIGGER_AS_BUTTONS, TRUE },
		{ CFG_PARAM_DPAD_AS_BUTTONS, GET_ELEMENT(GtkWidget, chkbtn_dpad_as_buttons), RNTF_DPAD_AS_BUTTONS, TRUE },
		{ CFG_PARAM_DPAD_AS_AXES, GET_ELEMENT(GtkWidget, chkbtn_dpad_as_axes), RNTF_DPAD_AS_AXES, TRUE },
		{ CFG_PARAM_MOUSE_INVERT_SCROLL, GET_ELEMENT(GtkWidget, chkbtn_mouse_invert_scroll), RNTF_MOUSE_INVERT_SCROLL, TRUE },
		{ CFG_PARAM_SWAP_STICKS, GET_ELEMENT(GtkWidget, chkbtn_swap_rl_sticks), RNTF_SWAP_RL_STICKS, TRUE },
		{ CFG_PARAM_ENABLE_NUNCHUK_X_ACCEL, GET_ELEMENT(GtkWidget, chkbtn_enable_nunchuk_x_accel), RNTF_NUNCHUK_ACC_ENABLE, TRUE },
		{ CFG_PARAM_ENABLE_NUNCHUK_Y_ACCEL, GET_ELEMENT(GtkWidget, chkbtn_enable_nunchuk_y_accel), RNTF_NUNCHUK_ACC_ENABLE, TRUE },
		{ CFG_PARAM_ENABLE_NUNCHUK_Z_ACCEL, GET_ELEMENT(GtkWidget, chkbtn_enable_nunchuk_z_accel), RNTF_NUNCHUK_ACC_ENABLE, TRUE },
		{ CFG_PARAM_DISABLE_ANALOG_TRIGGERS, GET_ELEMENT(GtkWidget, chkbtn_disable_analog_triggers), RNTF_DISABLE_ANALOG_TRIGGERS, TRUE },
		{ CFG_PARAM_AUTO_ENABLE_ANALOG, GET_ELEMENT(GtkWidget, chkbtn_auto_enable_analog), RNTF_AUTO_ENABLE_ANALOG, TRUE },
		{ },
	};

	GET_UI_ELEMENT(GtkLabel, label_product_name);
	GET_UI_ELEMENT(GtkLabel, label_firmware_version);
	GET_UI_ELEMENT(GtkLabel, label_usb_id);
	GET_UI_ELEMENT(GtkLabel, label_device_path);
	GET_UI_ELEMENT(GtkLabel, label_n_ports);
	GET_UI_ELEMENT(GtkSpinButton, pollInterval0);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_1p_joystick_mode);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_2p_joystick_mode);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_3p_joystick_mode);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_4p_joystick_mode);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_5p_joystick_mode);
	GET_UI_ELEMENT(GtkRadioButton, rbtn_mouse_mode);
	int i;
	struct rnt_adap_info *info = &app->current_adapter_info;
	char adap_sig[64];
	char ports_str[32];
	int cur_mode = -1;
	struct rnt_dyn_features features;

	if (!app->current_adapter_handle) {
		deselect_adapter(app);
		return;
	}


	if (1 == rnt_getConfig(app->current_adapter_handle, CFG_PARAM_MODE, buf, sizeof(buf))) {
		cur_mode = buf[0];

		if (info->caps.features & RNTF_DYNAMIC_FEATURES) {
			if (0 == rnt_getSupportedFeatures(app->current_adapter_handle, &features)) {
				struct {
					GtkRadioButton *w;
					uint8_t mode;
				} availableModes[] = {
					{	rbtn_1p_joystick_mode, CFG_MODE_STANDARD	},
					{	rbtn_2p_joystick_mode, CFG_MODE_2P_STANDARD },
					{	rbtn_3p_joystick_mode, CFG_MODE_3P_STANDARD },
					{	rbtn_4p_joystick_mode, CFG_MODE_4P_STANDARD },
					{	rbtn_5p_joystick_mode, CFG_MODE_5P_STANDARD	},
					{	rbtn_mouse_mode, CFG_MODE_MOUSE },
					{	}
				};
				for (i=0; availableModes[i].w; i++) {
					int available = 0;
					if (memchr(features.supported_modes, availableModes[i].mode, features.n_supported_modes)) {
						available = 1;
					}
					if (available) {
						printf("Mode %d available\n", availableModes[i].mode);
						gtk_widget_show(GTK_WIDGET(availableModes[i].w));
					} else {
						printf("Mode %d unavailable\n", availableModes[i].mode);
						gtk_widget_hide(GTK_WIDGET(availableModes[i].w));
					}
				}
			}
		}

		switch(cur_mode)
		{
			case CFG_MODE_STANDARD:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_1p_joystick_mode), buf[0]);
				break;
			case CFG_MODE_2P_STANDARD:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_2p_joystick_mode), buf[0]);
				break;
			case CFG_MODE_3P_STANDARD:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_3p_joystick_mode), buf[0]);
				break;
			case CFG_MODE_4P_STANDARD:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_4p_joystick_mode), buf[0]);
				break;
			case CFG_MODE_5P_STANDARD:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_5p_joystick_mode), buf[0]);
				break;
			case CFG_MODE_MOUSE:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbtn_mouse_mode), buf[0]);
				break;
		}
	}

	if (app->current_adapter_info.caps.features == 0) {
		gtk_widget_show(GET_ELEMENT(GtkWidget, lbl_no_configurable_params));
	} else {
		gtk_widget_hide(GET_ELEMENT(GtkWidget, lbl_no_configurable_params));
	}

	if (rnt_getSignature(app->current_adapter_handle, adap_sig, sizeof(adap_sig))) {
	} else {
		printf("Adapter signature: %s\n", adap_sig);
	}

	if (app->current_adapter_info.caps.features & RNTF_POLL_RATE) {
		n = rnt_getConfig(app->current_adapter_handle, CFG_PARAM_POLL_INTERVAL0, buf, sizeof(buf));
		if (n == 1) {
			printf("poll interval: %d\n", buf[0]);
			gtk_spin_button_set_value(pollInterval0, (gdouble)buf[0]);
		}
	}

	if (app->current_adapter_info.caps.min_poll_interval) {
		gtk_spin_button_set_range(pollInterval0, (gdouble)app->current_adapter_info.caps.min_poll_interval, 40);
	} else {
		gtk_spin_button_set_range(pollInterval0, 1, 40);
	}

	for (i=0; configurable_bits[i].widget; i++) {
		int avail = 0;

		// Decide if the widget (button or toggle button) is "available" given the adapter
		// features.
		if (configurable_bits[i].feature_bit) {
			if (app->current_adapter_info.caps.features & configurable_bits[i].feature_bit) {
				avail = 1;
			}
		}
		else {
			avail = 1;
		}

		if (avail) {
			gtk_widget_show(configurable_bits[i].widget);
			gtk_widget_set_sensitive(configurable_bits[i].widget, TRUE);

			if (GTK_IS_TOGGLE_BUTTON(configurable_bits[i].widget)) {
				rnt_getConfig(app->current_adapter_handle, configurable_bits[i].cfg_param, buf, sizeof(buf));
				printf("config param 0x%02x is %d\n",  configurable_bits[i].cfg_param, buf[0]);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configurable_bits[i].widget), buf[0]);
			}
		} else {
			// if non available, widget cannot be used
			gtk_widget_set_sensitive(configurable_bits[i].widget, FALSE);
			// non-availability may mean we must also hide the widget
			if (configurable_bits[i].hide_when_unavailable) {
				gtk_widget_hide(configurable_bits[i].widget);
			}
		}
	}

	if (sizeof(wchar_t)==4) {
		gtk_label_set_text(label_product_name, g_ucs4_to_utf8((void*)info->str_prodname, -1, NULL, NULL, NULL));
	} else {
		gtk_label_set_text(label_product_name, g_utf16_to_utf8((void*)info->str_prodname, -1, NULL, NULL, NULL));
	}

	if (0 == rnt_getVersion(app->current_adapter_handle, (char*)buf, sizeof(buf))) {
		sscanf((char*)buf, "%d.%d.%d", &app->firmware_maj, &app->firmware_min, &app->firmware_build);
		gtk_label_set_text(label_firmware_version, (char*)buf);

	}

	snprintf((char*)buf, sizeof(buf), "%04x:%04x", info->usb_vid, info->usb_pid);
	gtk_label_set_text(label_usb_id, (char*)buf);

	gtk_label_set_text(label_device_path, info->str_path);

	sprintf(ports_str, "%d", info->caps.n_channels);
	gtk_label_set_text(label_n_ports, ports_str);

	periodic_updater(app);
}

G_MODULE_EXPORT void pollIntervalChanged(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkSpinButton, pollInterval0);
	gdouble value;
	int n;
	unsigned char buf;

	value = gtk_spin_button_get_value(pollInterval0);
	printf("Value: %d\n", (int)value);
	buf = (int)value;

	if (app->current_adapter_info.caps.features & RNTF_POLL_RATE) {
		n = rnt_setConfig(app->current_adapter_handle, CFG_PARAM_POLL_INTERVAL0, &buf, 1);
		if (n != 0) {
			errorPopup(app, "Error setting configuration");
			deselect_adapter(app);
			rebuild_device_list_store(data, NULL);
		}
	}
}

G_MODULE_EXPORT void reset_adapter(GtkButton *button, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkDialog, dialog_please_wait);
	GTimer *reset_timer;

	// Display the please wait dialog (blocks the main window)
	gtk_widget_show(GTK_WIDGET(dialog_please_wait));

	rnt_reset(app->current_adapter_handle);
	deselect_adapter(app);

	reset_timer = g_timer_new();
	g_timer_start(reset_timer);

	/* Do nothing for two seconds */
	while (g_timer_elapsed(reset_timer, NULL) < 2.0) {
		while (gtk_events_pending()) {
			gtk_main_iteration_do(FALSE);
		}
	}

	// Try to reopen the same adapter. The serial number is still in app->current_adapter_info...
	rebuild_device_list_store(data, app->current_adapter_info.str_serial);

	syncGuiToCurrentAdapter(app);
	gtk_widget_hide(GTK_WIDGET(dialog_please_wait));
}


G_MODULE_EXPORT void cfg_adapter_mode_changed(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	struct {
		uint8_t config_value;
		GtkRadioButton *button;
	} modeButtons[] = {
		{ CFG_MODE_STANDARD, GET_ELEMENT(GtkRadioButton, rbtn_1p_joystick_mode) },
		{ CFG_MODE_2P_STANDARD, GET_ELEMENT(GtkRadioButton, rbtn_2p_joystick_mode) },
		{ CFG_MODE_3P_STANDARD, GET_ELEMENT(GtkRadioButton, rbtn_3p_joystick_mode) },
		{ CFG_MODE_4P_STANDARD, GET_ELEMENT(GtkRadioButton, rbtn_4p_joystick_mode) },
		{ CFG_MODE_5P_STANDARD, GET_ELEMENT(GtkRadioButton, rbtn_5p_joystick_mode) },
		{ CFG_MODE_MOUSE, GET_ELEMENT(GtkRadioButton, rbtn_mouse_mode) },
		{ },
	};
	int i;
	unsigned char buf[32];
	int current_config, next_config = -1;

//	printf("Adapter mode changed\n");

	if (1 != rnt_getConfig(app->current_adapter_handle, CFG_PARAM_MODE, buf, sizeof(buf))) {
		printf("Could not read current mode\n");
		return;
	}
	current_config = buf[0];
//	printf("Current config: %d\n", current_config);

	for (i=0; modeButtons[i].button; i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modeButtons[i].button))) {
			next_config = modeButtons[i].config_value;
			break;
		}
	}

	if (next_config == -1) {
		return;
	}

	if (next_config == current_config) {
		return;
	}

	printf("Changing adapter mode to: %d\n", next_config);
	buf[0] = next_config;
	rnt_setConfig(app->current_adapter_handle, CFG_PARAM_MODE, buf, 1);

	reset_adapter(NULL, data);
}

G_MODULE_EXPORT void config_checkbox_changed(GtkWidget *win, gpointer data)
{
	struct application *app = data;
	struct {
		unsigned char cfg_param;
		GtkToggleButton *chkbtn;
	} configurable_bits[] = {
		{ CFG_PARAM_FULL_SLIDERS, GET_ELEMENT(GtkToggleButton, chkbtn_gc_full_sliders) },
		{ CFG_PARAM_INVERT_TRIG, GET_ELEMENT(GtkToggleButton, chkbtn_gc_invert_trig) },
		{ CFG_PARAM_TRIGGERS_AS_BUTTONS, GET_ELEMENT(GtkToggleButton, chkbtn_sliders_as_buttons) },
		{ CFG_PARAM_DPAD_AS_BUTTONS, GET_ELEMENT(GtkToggleButton, chkbtn_dpad_as_buttons) },
		{ CFG_PARAM_DPAD_AS_AXES, GET_ELEMENT(GtkToggleButton, chkbtn_dpad_as_axes) },
		{ CFG_PARAM_MOUSE_INVERT_SCROLL, GET_ELEMENT(GtkToggleButton, chkbtn_mouse_invert_scroll) },
		{ CFG_PARAM_SWAP_STICKS, GET_ELEMENT(GtkToggleButton, chkbtn_swap_rl_sticks) },
		{ CFG_PARAM_ENABLE_NUNCHUK_X_ACCEL, GET_ELEMENT(GtkToggleButton, chkbtn_enable_nunchuk_x_accel) },
		{ CFG_PARAM_ENABLE_NUNCHUK_Y_ACCEL, GET_ELEMENT(GtkToggleButton, chkbtn_enable_nunchuk_y_accel) },
		{ CFG_PARAM_ENABLE_NUNCHUK_Z_ACCEL, GET_ELEMENT(GtkToggleButton, chkbtn_enable_nunchuk_z_accel) },
		{ CFG_PARAM_DISABLE_ANALOG_TRIGGERS, GET_ELEMENT(GtkToggleButton, chkbtn_disable_analog_triggers) },
		{ CFG_PARAM_AUTO_ENABLE_ANALOG, GET_ELEMENT(GtkToggleButton, chkbtn_auto_enable_analog) },
		{ },
	};
	int i, n;
	unsigned char buf;

	for (i=0; configurable_bits[i].chkbtn; i++) {
		if (configurable_bits[i].chkbtn != GTK_TOGGLE_BUTTON(win))
			continue;

		buf = gtk_toggle_button_get_active(configurable_bits[i].chkbtn);
		n = rnt_setConfig(app->current_adapter_handle, configurable_bits[i].cfg_param, &buf, 1);
		if (n != 0) {
			errorPopup(app, "Error setting configuration");
			deselect_adapter(app);
			rebuild_device_list_store(app, NULL);
			break;
		}
		printf("cfg param 0x%02x now set to %d\n", configurable_bits[i].cfg_param, buf);
	}
}

gboolean rebuild_device_list_store(gpointer data, wchar_t *auto_select_serial)
{
	struct application *app = data;
	struct rnt_adap_list_ctx *listctx;
	struct rnt_adap_info info;
	GtkListStore *list_store;
	GET_UI_ELEMENT(GtkComboBox, cb_adapter_list);

	list_store = GTK_LIST_STORE( gtk_builder_get_object(app->builder, "adaptersList") );

	gtk_list_store_clear(list_store);

	printf("Listing device...\n");
	listctx = rnt_allocListCtx();
	if (!listctx)
		return FALSE;

	while (rnt_listDevices(&info, listctx)) {
		GtkTreeIter iter;
		printf("Device '%ls'\n", info.str_prodname);
		gtk_list_store_append(list_store, &iter);
		if (sizeof(wchar_t)==4) {
			gtk_list_store_set(list_store, &iter,
							0, g_ucs4_to_utf8((void*)info.str_serial, -1, NULL, NULL, NULL),
							1, g_ucs4_to_utf8((void*)info.str_prodname, -1, NULL, NULL, NULL),
							3, g_memdup(&info, sizeof(struct rnt_adap_info)),
								-1);
		} else {
			gtk_list_store_set(list_store, &iter,
							0, g_utf16_to_utf8((void*)info.str_serial, -1, NULL, NULL, NULL),
							1, g_utf16_to_utf8((void*)info.str_prodname, -1, NULL, NULL, NULL),
							3, g_memdup(&info, sizeof(struct rnt_adap_info)),
								-1);
		}
		if (app->current_adapter_handle) {
			if (!wcscmp(app->current_adapter_info.str_serial, info.str_serial)) {
				gtk_combo_box_set_active_iter(cb_adapter_list, &iter);
			}
		} else if (auto_select_serial) {
			if (!wcscmp(auto_select_serial, info.str_serial)) {
				gtk_combo_box_set_active_iter(cb_adapter_list, &iter);
			}
		}
	}

	rnt_freeListCtx(listctx);
	return FALSE;
}

G_MODULE_EXPORT void onMainWindowShow(GtkWidget *win, gpointer data)
{
	int res;
	struct application *app = data;

	res = rnt_init(1);
	if (res) {
		GtkWidget *d = GTK_WIDGET( gtk_builder_get_object(app->builder, "internalError") );
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "rnt_init failed (returned %d)", res);
		gtk_widget_show(d);
		return;
	}

	rebuild_device_list_store(data, NULL);
}

G_MODULE_EXPORT void adapterSelected(GtkComboBox *cb, gpointer data)
{
	struct application *app = data;
	GtkTreeIter iter;
	GtkListStore *list_store = GTK_LIST_STORE( gtk_builder_get_object(app->builder, "adaptersList") );
	GtkWidget *adapter_details = GTK_WIDGET( gtk_builder_get_object(app->builder, "adapterDetails") );
	GET_UI_ELEMENT(GtkMenuItem, menu_manage_gc2n64);
	struct rnt_adap_info *info;

	if (app->current_adapter_handle) {
		rnt_closeDevice(app->current_adapter_handle);
		app->current_adapter_handle = NULL;

		desensitize_adapter_widgets(app);
	}

	if (gtk_combo_box_get_active_iter(cb, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 3, &info, -1);
		printf("%s\n", info->str_path);

		app->current_adapter_handle = rnt_openDevice(info);
		if (!app->current_adapter_handle) {
			errorPopup(app, "Failed to open adapter");
			deselect_adapter(app);
			return;
		}

		memcpy(&app->current_adapter_info, info, sizeof(struct rnt_adap_info));

		syncGuiToCurrentAdapter(app);
		gtk_widget_set_sensitive(adapter_details, TRUE);
		// TODO : Should only be available for adapters with an N64 port
		gtk_widget_set_sensitive((GtkWidget*)menu_manage_gc2n64, TRUE);
	}
}

G_MODULE_EXPORT void onMainWindowHide(GtkWidget *win, gpointer data)
{
	rnt_shutdown();
}

G_MODULE_EXPORT void suspend_polling(GtkButton *button, gpointer data)
{
	struct application *app = data;

	rnt_suspendPolling(app->current_adapter_handle, 1);
}

G_MODULE_EXPORT void resume_polling(GtkButton *button, gpointer data)
{
	struct application *app = data;

	rnt_suspendPolling(app->current_adapter_handle, 0);
}


G_MODULE_EXPORT void onFileRescan(GtkWidget *wid, gpointer data)
{
	rebuild_device_list_store(data, NULL);
}

static int mempak_io_progress_cb(int progress, void *ctx)
{
	struct application *app = ctx;
	GET_UI_ELEMENT(GtkProgressBar, mempak_io_progress);

	gtk_progress_bar_set_fraction(mempak_io_progress, progress/((gdouble)MEMPAK_MEM_SIZE));
	while (gtk_events_pending()) {
		gtk_main_iteration_do(FALSE);
	}

	return app->stop_mempak_io;
}

G_MODULE_EXPORT void mempak_io_stop(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	app->stop_mempak_io = 1;
}

G_MODULE_EXPORT void erase_n64_pak(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	uiio *u = getUIIO_gtk(NULL, app->mainwindow);

	if (!app->current_adapter_handle)
		return;

	u->caption = "Erasing Controller Pak...";

	mempak_fill(app->current_adapter_handle, 0, 0xFF, 0, u);
}

G_MODULE_EXPORT void write_n64_pak(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkWindow, win_mempak_edit);
	GET_UI_ELEMENT(GtkDialog, mempak_io_dialog);
	GET_UI_ELEMENT(GtkLabel, mempak_op_label);
	GtkWidget *confirm_dialog;
	gint res;

	gtk_widget_show(GTK_WIDGET(win_mempak_edit));


	confirm_dialog = gtk_message_dialog_new(GTK_WINDOW(win_mempak_edit), GTK_DIALOG_MODAL,
									GTK_MESSAGE_QUESTION, 0, "Your memory card will be completely overwritten by the content of the memory pack editor.\n\nAre you sure?");

	gtk_dialog_add_buttons(GTK_DIALOG(confirm_dialog), "Cancel", 1, "Continue", 2, NULL);

	res = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
	gtk_widget_destroy(confirm_dialog);

	switch(res)
	{
		case 2:
			printf("Confirmed write N64 mempak.\n");

			app->stop_mempak_io = 0;
			gtk_label_set_text(mempak_op_label, "Writing memory pack...");
			gtk_widget_show(GTK_WIDGET(mempak_io_dialog));

			rnt_suspendPolling(app->current_adapter_handle, 1);
			res = gcn64lib_mempak_upload(app->current_adapter_handle, 0, mpke_getCurrentMempak(app), mempak_io_progress_cb, app);
			rnt_suspendPolling(app->current_adapter_handle, 0);
			gtk_widget_hide(GTK_WIDGET(mempak_io_dialog));

			if (res != 0) {
				switch(res)
				{
					case -1: errorPopup(app, "No mempak detected"); break;
					case -2: errorPopup(app, "I/O error writing to mempak"); break;
					case -4: errorPopup(app, "Write aborted"); break;
					default:
						errorPopup(app, "Error writing to mempak"); break;
				}
			}

			break;
		case 1:
		default:
			printf("Write N64 mempak cancelled\n");
	}

}

G_MODULE_EXPORT void read_n64_pak(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	GET_UI_ELEMENT(GtkWindow, win_mempak_edit);
	GET_UI_ELEMENT(GtkDialog, mempak_io_dialog);
	GET_UI_ELEMENT(GtkLabel, mempak_op_label);
	mempak_structure_t *mpk;
	int res;

	printf("N64 read mempak\n");
	if (!app->current_adapter_handle)
		return;

	gtk_widget_show(GTK_WIDGET(mempak_io_dialog));
	gtk_label_set_text(mempak_op_label, "Reading memory pack...");

	app->stop_mempak_io = 0;
	rnt_suspendPolling(app->current_adapter_handle, 1);
	res = gcn64lib_mempak_download(app->current_adapter_handle, 0, &mpk, mempak_io_progress_cb, app);
	rnt_suspendPolling(app->current_adapter_handle, 0);

	gtk_widget_hide(GTK_WIDGET(mempak_io_dialog));
	if (res != 0) {
		switch(res)
		{
			case -1: errorPopup(app, "No mempak detected"); break;
			case -2: errorPopup(app, "I/O error reading mempak"); break;
			case -4: errorPopup(app, "Read aborted"); break;
			default:
			case -3: errorPopup(app, "Error reading mempak"); break;
		}
	}
	else {
		mpke_replaceMpk(app, mpk, NULL);
		gtk_widget_show(GTK_WIDGET(win_mempak_edit));
	}
}

G_MODULE_EXPORT void testVibration(GtkWidget *wid, gpointer data)
{
	struct application *app = data;
	GtkWidget *dialog;

	if ((app->firmware_maj < 3) || (app->firmware_min < 1)) {
		errorPopup(app, "Firmware 3.1 or later required");
	} else {
		if (0 > rnt_forceVibration(app->current_adapter_handle, 0, 1)) {
			errorPopup(app, "Error setting vibration");
		} else {
			dialog = gtk_dialog_new_with_buttons("Vibration test", app->mainwindow, GTK_DIALOG_MODAL, "Stop vibration", GTK_RESPONSE_ACCEPT, NULL);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);

			rnt_forceVibration(app->current_adapter_handle, 0, 0);
		}
	}
}

int
main( int    argc,
      char **argv )
{
    GtkWindow  *window;
    GError     *error = NULL;
	struct application app = { };

#ifdef WINDOWS
    /* Hack to disable non-standard title bar under windows */
    putenv("GTK_CSD=0");

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

    /* Init GTK+ */
    gtk_init( &argc, &argv );

    /* Create new GtkBuilder object */
    app.builder = gtk_builder_new();
    /* Load UI from file. If error occurs, report it and quit application.
     * Replace "tut.glade" with your saved project. */
    if( ! gtk_builder_add_from_resource( app.builder, "/com/raphnet/gcn64ctl_gui/gui.xml", &error ) )
    {
        g_warning( "%s", error->message );
        g_free( error );
        return( 1 );
    }

	app.mpke = mpkedit_new(&app);

    /* Get main window pointer from UI */
    window = GTK_WINDOW( gtk_builder_get_object( app.builder, "mainWindow" ) );
	app.mainwindow = window;

    /* Connect signals */
    gtk_builder_connect_signals( app.builder, &app );

	g_timeout_add_seconds(1, periodic_updater, &app);

    /* Show window. All other widgets are automatically shown by GtkBuilder */
    gtk_widget_show( GTK_WIDGET(window) );

	gtk_about_dialog_set_version((GtkAboutDialog*)gtk_builder_get_object(app.builder, "aboutdialog1"), VERSION_STR);

	gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(app.builder, "link_raphnet_website")), "www.raphnet-tech.com");

    /* Start main loop */
    gtk_main();

	mpkedit_free(app.mpke);

    return( 0 );
}
