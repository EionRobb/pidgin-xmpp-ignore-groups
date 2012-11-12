#define PURPLE_PLUGINS

#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

// GNU C libraries
#include <stdio.h>
#include <string.h>

// Glib
#include <glib.h>

// Libpurple functions
#include "account.h"
#include "accountopt.h"
#include "debug.h"

#ifndef _
#define _(a) (a)
#define N_(a) (a)
#endif

extern void jabber_iq_signal_register(const gchar *node, const gchar *xmlns);
extern void jabber_iq_signal_unregister(const gchar *node, const gchar *xmlns);

static gboolean
handle_jabber_receiving_iq_signal(PurpleConnection *pc, const char *type, const char *id, const char *from, xmlnode *iq)
{
	PurpleAccount *account = purple_connection_get_account(pc);
	xmlnode *query, *item, *group;
	
	// Remove groups if we're not using them
	if ((query = xmlnode_get_child(iq, "query")) && g_str_equal(xmlnode_get_namespace(query), "jabber:iq:roster"))
	{
		for(item = xmlnode_get_child(query, "item"); item; item = xmlnode_get_next_twin(item))
		{
			if (purple_account_get_bool(account, "ignore_groups", FALSE))
			{
				for(group = xmlnode_get_child(item, "group"); group; group = xmlnode_get_child(item, "group")) {
					xmlnode_free(group);
				}
			}
		}
		if (purple_account_get_bool(account, "ignore_groups", FALSE))
		{
			purple_debug_info("xmpp-ignore-groups", "Just disabled groups/friend lists\n");
		}
	}
	
	return FALSE;
}

static gboolean load_plugin(PurplePlugin *plugin)
{
	PurplePlugin *xmpp_prpl;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	
	purple_debug_info("xmpp-ignore-groups", "plugin_load %s\n", purple_plugin_get_id(plugin));
	
	xmpp_prpl = purple_plugins_find_with_id("prpl-jabber");
	if (xmpp_prpl == NULL)
		return FALSE;
	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(xmpp_prpl);
	if (prpl_info == NULL)
		return FALSE;
	
	//intercept iq in case it has roster info with groups
	jabber_iq_signal_register("query", "jabber:iq:roster");
	purple_signal_connect(xmpp_prpl, "jabber-receiving-iq", plugin,
						  PURPLE_CALLBACK(handle_jabber_receiving_iq_signal), NULL);
	
	/* Add options to the advanced screen in the account settings */
	option = purple_account_option_bool_new(_("Ignore server-sent groups"), "ignore_groups", FALSE);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	return TRUE;
}

static gboolean unload_plugin(PurplePlugin *plugin)
{
	GList *list;
	PurplePlugin *xmpp_prpl;
	PurplePluginProtocolInfo *prpl_info;

	purple_signals_disconnect_by_handle(plugin);
	
	jabber_iq_signal_unregister("query", "jabber:iq:roster");
	
	xmpp_prpl = purple_plugins_find_with_id("prpl-jabber");
	if (xmpp_prpl == NULL)
		return FALSE;
	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(xmpp_prpl);
	if (prpl_info == NULL)
		return FALSE;
	
	for(list = prpl_info->protocol_options;
		list != NULL;
		list = g_list_next(list))
	{
		PurpleAccountOption *option = (PurpleAccountOption *) list->data;
		if (g_str_equal(purple_account_option_get_setting(option), "ignore_groups"))
		{
			list = g_list_delete_link(list, list);
			purple_account_option_destroy(option);
			break;
		}
	}
	
	return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
	PurplePluginInfo *info = plugin->info;
	
	// We rely on the Jabber/XMPP protocol plugin to work
	info->dependencies = g_list_prepend(info->dependencies, "prpl-jabber");
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	2,
	7,
	PURPLE_PLUGIN_STANDARD,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                            /**< priority       */

	"core-eionrobb-xmpp-ignore-groups",                                    /**< id             */
	"XMPP Ignore Groups",                                           /**< name           */
	"1.0",                                  /**< version        */
	                                                  /**  summary        */
	N_("Ignore server-sent XMPP groups"),
	                                                  /**  description    */
	N_("Used if a server sends it's own roster that forces buddy groups on you, and you don't want them.  After enabling this plugin, enable the 'Ignore Groups' option in the advanced settings of the XMPP account"),
	"Eion Robb <eionrobb@gmail.com>",                  /**< author         */
	"http://xmpp-ignore-groups.googlecode.com/",                                     /**< homepage       */

	load_plugin,                                      /**< load           */
	unload_plugin,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                       /**< extra_info     */
	NULL,                                             /**< prefs_info     */
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(xmppignoregroups, plugin_init, info);
