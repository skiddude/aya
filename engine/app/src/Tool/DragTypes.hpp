

#pragma once

#ifndef AYA_DRAG_ENUMS_DEFINED
#define AYA_DRAG_ENUMS_DEFINED

namespace Aya
{
namespace DRAG
{

typedef enum
{
    NO_UNJOIN_NO_JOIN,
    UNJOIN_JOIN,
    UNJOIN_NO_JOIN
} JoinType;

typedef enum
{
    MOVE_DROP,
    MOVE_NO_DROP
} MoveType;

typedef enum
{
    WEAK_MANUAL_JOINT,
    STRONG_MANUAL_JOINT,
    INFINITE_MANUAL_JOINT
} ManualJointType;

typedef enum
{
    ONE_STUD,
    QUARTER_STUD,
    OFF
} DraggerGridMode;

} // namespace DRAG
} // namespace Aya
#endif