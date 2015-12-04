#pragma once

#include <vector>
#include "logicalaccess/services/cardservice.hpp"

namespace logicalaccess
{
/**
 * This service operate mostly on "backup cards" whose UID
 * can be changed.
 */
class UIDChangerService : public CardService
{
  public:
    UIDChangerService(std::shared_ptr<Chip> chip);
    virtual ~UIDChangerService() = default;

    virtual void changeUID(const std::vector<uint8_t> &new_uid) = 0;

    virtual CardServiceType getServiceType() const override;
};
}
