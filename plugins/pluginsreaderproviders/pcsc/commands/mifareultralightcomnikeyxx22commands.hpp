/**
* \file mifareultralightcomnikeyxx22commands.hpp
* \author Maxime C. <maxime-dev@islog.com>
* \brief Mifare Ultralight C - Omnikey xx22.
*/

#ifndef LOGICALACCESS_MIFAREULTRALIGHTCOMNIKEYXX22COMMANDS_HPP
#define LOGICALACCESS_MIFAREULTRALIGHTCOMNIKEYXX22COMMANDS_HPP

#include "mifareultralightcpcsccommands.hpp"

namespace logicalaccess
{
#define CMD_MIFAREULTRALIGHTCOMNIKEYXX22 "MifareUltralightCOmnikeyXX22"

/**
* \brief The Mifare Ultralight C commands class for Omnikey xx22 reader.
*/
class LIBLOGICALACCESS_API MifareUltralightCOmnikeyXX22Commands
    : public MifareUltralightPCSCCommands
#ifndef SWIG
      ,
      public MifareUltralightCCommands
#endif
{
  public:
    /**
    * \brief Constructor.
    */
    MifareUltralightCOmnikeyXX22Commands();

    explicit MifareUltralightCOmnikeyXX22Commands(std::string);

    /**
    * \brief Destructor.
    */
    virtual ~MifareUltralightCOmnikeyXX22Commands();

    /**
    * \brief Authenticate to the chip.
    * \param authkey The authentication key.
    */
    void authenticate(std::shared_ptr<TripleDESKey> authkey) override;

    std::shared_ptr<MifareUltralightChip> getMifareUltralightChip() override;

    void writePage(int page, const ByteVector &buf) override;
};
}

#endif /* LOGICALACCESS_MIFAREULTRALIGHTCOMNIKEYXX22COMMANDS_HPP */