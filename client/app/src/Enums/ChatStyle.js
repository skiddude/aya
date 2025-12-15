const ChatStyle = {
    Classic: 0,
    Bubble: 1,
    ClassicAndBubble: 2
}

const names = {
    [ChatStyle.Classic]: "Classic",
    [ChatStyle.Bubble]: "Bubble",
    [ChatStyle.ClassicAndBubble]: "Classic and Bubble"
}

function getChatStyleName(style) {
    return names[style]
}

export { ChatStyle, getChatStyleName }
