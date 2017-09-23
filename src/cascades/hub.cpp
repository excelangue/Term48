#include "hub.h"

#include <bb/platform/Notification>
#include <bb/platform/NotificationDefaultApplicationSettings>

bool notify_instantpreview(const char *message) {
  using namespace bb::platform;
  
  // Clear any existing notifications
  Notification::clearEffectsForAll();
  Notification::deleteAllFromInbox();
 
  // Turn on previews in the Settings app
  bb::platform::NotificationDefaultApplicationSettings settings;
  settings.setPreview(bb::platform::NotificationPriorityPolicy::Allow);
  settings.apply();
 
  Notification* pNotification = new Notification();
  pNotification->setType(NotificationType::Default);
  pNotification->setTitle("Term48 Alert");
  pNotification->setBody(message);
  pNotification->setType(NotificationType::AllAlertsOff);
  pNotification->notify();

  return true;
}
