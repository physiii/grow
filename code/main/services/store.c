#include "storage.c"
char store_service_message[1000];
bool store_service_message_ready = false;
cJSON *auth_uids = NULL;

int store_state(cJSON * state)
{
  char *state_str = cJSON_PrintUnformatted(state);
  store_char("state", state_str);
  free(state_str);
  return 0;
}

int restore_state()
{
  char state_str[2000];
  strcpy(state_str, get_char("state"));
  if (strcmp(state_str,"")==0) {
    printf("State not found, initializing...\n");
    snprintf(state_str,sizeof(state_str),""
    "{\"light_level\":51, \"time\":1, \"cycletime\":1, \"start_time\":1, \"on\":false, \"ph\":2.1, \"atm_temp\":75, \"humidity\":90, \"water_temp\":75, \"ec\":0.000, \"pco2\":4.1}");
  }
  state = cJSON_Parse(state_str);
  return 0;
}

int store_uids(cJSON * uids)
{
  printf("Storing UIDs: %s\n",cJSON_PrintUnformatted(uids));
  store_char("uids", cJSON_PrintUnformatted(uids));
  return 0;
}

int load_uids_from_flash()
{
  char *uids = get_char("uids");
  if (strcmp(uids,"")==0) {
    printf("nfc_uids not found in flash.\n");
    auth_uids = cJSON_CreateArray();
    return 1;
  } else {
    printf("nfc_uids found in flash.\n%s\n",uids);
  }

  cJSON *obj = cJSON_Parse(uids);
  if (cJSON_IsArray(obj)) {
    auth_uids = obj;
    printf("Loaded nfc_uids from flash. %s\n", uids);
  } else {
    printf("nfc_uids are not in a json array\n");
  }

  return 0;
}

void add_auth_uid (char * new_id)
{
  cJSON *id =  NULL;
  char current_id[50];

  cJSON_ArrayForEach(id, auth_uids) {
    if (cJSON_IsString(id)) {
      strcpy(current_id,id->valuestring);
      printf("Current UID is %s.\n",current_id);
      if (strcmp(current_id, new_id)==0) {
        printf("UID Already added.\n");
        return;
      }
    }
  }
  cJSON *id_obj =  cJSON_CreateString(new_id);
  cJSON_AddItemToArray(auth_uids, id_obj);
  store_uids(auth_uids);
}

void remove_auth_uid (char * target_id)
{
  cJSON *new_auth_uids = cJSON_CreateArray();
  cJSON *id =  NULL;
  char current_id[50];

  cJSON_ArrayForEach(id, auth_uids) {
    // if (cJSON_IsString(id)) {
      strcpy(current_id,id->valuestring);
      // printf("Current UID is %s.\n",current_id);
      if (strcmp(current_id, target_id)==0) {
        printf("Found match for target %s...%s\n", target_id, current_id);
      } else {
        cJSON_AddItemToArray(new_auth_uids,id);
        printf("Add UID %s.\n",current_id);
      }
    // }
  }

  store_uids(new_auth_uids);
}

bool is_uid_authorized (char * uid)
{
  cJSON *uid_obj =  NULL;
  char current_uid[20];

  cJSON_ArrayForEach(uid_obj, auth_uids) {
    sprintf(current_uid,"%s",uid_obj->valuestring);
    if (strcmp(current_uid,uid)==0) {
      return true;
    }
  }

  return false;
}

void store_main()
{
  printf("starting store service\n");
  load_uids_from_flash();
  // TaskHandle_t store_service_task;
  // xTaskCreate(&store_service, "store_service_task", 5000, NULL, 5, NULL);
}
