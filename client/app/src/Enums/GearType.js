const GearType = {
    MeleeWeapons: 0,
    RangedWeapons: 1,
    Explosives: 2,
    PowerUps: 3,
    NavigationEnhancers: 4,
    MusicalInstruments: 5,
    SocialItems: 6,
    BuildingTools: 7,
    Transport: 8
}

const names = {
    [GearType.MeleeWeapons]: "Melee Weapons",
    [GearType.RangedWeapons]: "Ranged Weapons",
    [GearType.Explosives]: "Explosives",
    [GearType.PowerUps]: "Power Ups",
    [GearType.NavigationEnhancers]: "Navigation Enhancers",
    [GearType.MusicalInstruments]: "Musical Instruments",
    [GearType.SocialItems]: "Social Items",
    [GearType.BuildingTools]: "Building Tools",
    [GearType.Transport]: "Transport"
}

const icons = {
    [GearType.MeleeWeapons]: "sword",
    [GearType.RangedWeapons]: "crosshairs",
    [GearType.Explosives]: "bomb",
    [GearType.PowerUps]: "bolt",
    [GearType.NavigationEnhancers]: "compass",
    [GearType.MusicalInstruments]: "music",
    [GearType.SocialItems]: "share-nodes",
    [GearType.BuildingTools]: "screwdriver-wrench",
    [GearType.Transport]: "car-side"
}

function getGearTypeName(type) {
    return names[type]
}

function getGearTypeIcon(type) {
    return icons[type]
}

function enableGearType(gearAttributes, gearType) {
    return gearAttributes | (1 << gearType)
}

function disableGearType(gearAttributes, gearType) {
    return gearAttributes & ~(1 << gearType)
}

function isGearTypeEnabled(gearAttributes, gearType) {
    return (gearAttributes & (1 << gearType)) !== 0
}

export { GearType, getGearTypeName, getGearTypeIcon, enableGearType, disableGearType, isGearTypeEnabled }
