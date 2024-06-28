#pragma once

enum class LocalStatusCode {
    Ok,
    HandshakeAnswerHeaderNotEqual,
    WrongStartWord,
    WrongId,
    WrongFlags,
    BufferToSmall,
    WrongHash,
    NothingToWrite,
    LoadAnswerNotEqual,
    Timeout,
};
