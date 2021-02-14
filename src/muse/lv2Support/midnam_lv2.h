#define LV2_MIDNAM_URI "http://ardour.org/lv2/midnam"
#define LV2_MIDNAM_PREFIX LV2_MIDNAM_URI "#"
#define LV2_MIDNAM__interface LV2_MIDNAM_PREFIX "interface"
#define LV2_MIDNAM__update LV2_MIDNAM_PREFIX "update"

typedef void* LV2_Midnam_Handle;

/** a LV2 Feature provided by the Host to the plugin */
typedef struct {
	/** Opaque host data */
	LV2_Midnam_Handle handle;
	/** Request from run() that the host should re-read the midnam */
	void (*update)(LV2_Midnam_Handle handle);
} LV2_Midnam;

typedef struct {
	/** Query midnam document. The plugin
	 * is expected to return a null-terminated XML
	 * text which is a valid midnam desciption
	 * (or NULL in case of error).
	 *
	 * The midnam <Model> must be unique and
	 * specific for the given plugin-instance.
	 */
	char* (*midnam)(LV2_Handle instance);

	/** The unique model id used ith the midnam,
	 * (or NULL).
	 */
	char* (*model)(LV2_Handle instance);

	/** free allocated strings. The host
	 * calls this for every value returned by
	 * \ref midnam and \ref model.
	 */
	void (*free)(char*);
} LV2_Midnam_Interface;
