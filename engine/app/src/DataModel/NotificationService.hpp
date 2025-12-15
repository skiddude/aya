

#pragma once

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"

namespace Aya
{
extern const char* const sNotificationService;
class NotificationService
    : public DescribedNonCreatable<NotificationService, Instance, sNotificationService, Reflection::ClassDescriptor::INTERNAL>
    , public Service
{
private:
    typedef DescribedNonCreatable<NotificationService, Instance, sNotificationService, Reflection::ClassDescriptor::INTERNAL> Super;

    bool canUseService();

public:
    NotificationService();

    Aya::signal<void(int, int, std::string, int)> scheduleNotificationSignal;
    Aya::signal<void(int, int)> cancelNotificationSignal;
    Aya::signal<void(int)> cancelAllNotificationSignal;
    Aya::signal<void(int, boost::function<void(shared_ptr<const Reflection::ValueArray>)>, boost::function<void(std::string)>)>
        getScheduledNotificationsSignal;


    void scheduleNotification(int userId, int alertId, std::string alerMsg, int minutesToFire);
    void cancelNotification(int userId, int alertId);
    void cancelAllNotification(int userId);
    void getScheduledNotifications(
        int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
};
} // namespace Aya