#include "P2PObject.h"

bool FListSession::operator==(const FListSession& Other)
{
	return
		IP == Other.IP &&
		Port == Other.Port &&
		ID == Other.ID &&
		UsePassword == Other.UsePassword &&
		Property == Other.Property;
}
