#include <Commands.h>
#include <FuncConstructor.hpp>

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>

#include <IGApi/SessionManager.hpp>
#include <dev_tools/commons/HelpPage.hpp>

namespace fs = std::filesystem;

// ============================================================================
// Helper: Check prerequisites (logged in + target set)
// ============================================================================
static bool checkReady(const std::string& cmd) {
    auto& mgr = IG::SessionManager::Instance();

    if (!mgr.IsLoggedIn()) {
        std::cerr << "[!] " << cmd << ": You must be logged in first" << std::endl;
        std::cerr << "[!] Use 'userctl login <username>' to authenticate" << std::endl;
        return false;
    }

    if (!mgr.HasTarget()) {
        std::cerr << "[!] " << cmd << ": No target set" << std::endl;
        std::cerr << "[!] Use 'sessionctl target <username>' to set a target" << std::endl;
        return false;
    }

    return true;
}

static std::string formatTimestamp(const std::string& ts) {
    try {
        long long epoch = std::stoll(ts);
        std::time_t t = static_cast<std::time_t>(epoch);
        std::tm* tm = std::localtime(&t);
        if (!tm) return ts;
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    } catch (...) {
        return ts;
    }
}

// ============================================================================
// Command: info
// ============================================================================
static int cmd_info(const std::vector<std::string>& args) {
    if (!checkReady("info")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << std::endl;
    std::cout << "=== Target Account Information ===" << std::endl;
    std::cout << std::endl;
    std::cout << "  Username:      @" << target.username << std::endl;
    std::cout << "  Full Name:     " << (target.fullName.empty() ? "(none)" : target.fullName) << std::endl;
    std::cout << "  User ID:       " << target.userId << std::endl;
    std::cout << "  Biography:     " << (target.biography.empty() ? "(none)" : target.biography) << std::endl;
    std::cout << "  External URL:  " << (target.externalUrl.empty() ? "(none)" : target.externalUrl) << std::endl;
    std::cout << "  Email:         " << (target.email.empty() ? "(none)" : target.email) << std::endl;
    std::cout << "  Phone:         " << (target.phoneNumber.empty() ? "(none)" : target.phoneNumber) << std::endl;
    std::cout << "  Category:      " << (target.category.empty() ? "(none)" : target.category) << std::endl;
    std::cout << std::endl;
    std::cout << "  Posts:         " << target.mediaCount << std::endl;
    std::cout << "  Followers:     " << target.followerCount << std::endl;
    std::cout << "  Following:     " << target.followingCount << std::endl;
    std::cout << std::endl;
    std::cout << "  Private:       " << (target.isPrivate ? "Yes" : "No") << std::endl;
    std::cout << "  Verified:      " << (target.isVerified ? "Yes" : "No") << std::endl;
    std::cout << "  Business:      " << (target.isBusiness ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    std::cout << "  Profile Pic:   " << (target.profilePicUrl.empty() ? "(none)" : target.profilePicUrl) << std::endl;
    std::cout << std::endl;

    return 0;
}

// ============================================================================
// Command: followers
// ============================================================================
static int cmd_followers(const std::vector<std::string>& args) {
    if (!checkReady("followers")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxCount = 50;
    if (!args.empty()) {
        try { maxCount = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching followers for @" << target.username << " (max " << maxCount << ")..." << std::endl;

    auto followers = mgr.FetchFollowers(target.userId, maxCount);

    if (followers.empty()) {
        std::cout << "[*] No followers found (account may be private)" << std::endl;
        return 0;
    }

    std::cout << "[+] Found " << followers.size() << " follower(s):" << std::endl << std::endl;

    for (size_t i = 0; i < followers.size(); i++) {
        const auto& f = followers[i];
        std::cout << "  " << (i + 1) << ". @" << f.username;
        if (!f.fullName.empty())
            std::cout << " (" << f.fullName << ")";
        if (f.isVerified)
            std::cout << " [Verified]";
        if (f.isPrivate)
            std::cout << " [Private]";
        std::cout << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: followings
// ============================================================================
static int cmd_followings(const std::vector<std::string>& args) {
    if (!checkReady("followings")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxCount = 50;
    if (!args.empty()) {
        try { maxCount = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching following for @" << target.username << " (max " << maxCount << ")..." << std::endl;

    auto following = mgr.FetchFollowing(target.userId, maxCount);

    if (following.empty()) {
        std::cout << "[*] No following found (account may be private)" << std::endl;
        return 0;
    }

    std::cout << "[+] Found " << following.size() << " following:" << std::endl << std::endl;

    for (size_t i = 0; i < following.size(); i++) {
        const auto& f = following[i];
        std::cout << "  " << (i + 1) << ". @" << f.username;
        if (!f.fullName.empty())
            std::cout << " (" << f.fullName << ")";
        if (f.isVerified)
            std::cout << " [Verified]";
        if (f.isPrivate)
            std::cout << " [Private]";
        std::cout << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: likes - total likes across all posts
// ============================================================================
static int cmd_likes(const std::vector<std::string>& args) {
    if (!checkReady("likes")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 20;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching like counts for @" << target.username << " (last " << maxPosts << " posts)..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found (account may be private)" << std::endl;
        return 0;
    }

    int totalLikes = 0;
    std::cout << std::endl;

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];
        std::string caption = item.caption.substr(0, 50);
        if (item.caption.length() > 50) caption += "...";
        if (caption.empty()) caption = "(no caption)";

        // Replace newlines in caption preview
        std::replace(caption.begin(), caption.end(), '\n', ' ');

        std::cout << "  Post " << (i + 1) << ": " << item.likeCount << " likes"
                  << " | " << item.mediaType
                  << " | " << caption << std::endl;
        totalLikes += item.likeCount;
    }

    std::cout << std::endl;
    std::cout << "[+] Total likes across " << feed.size() << " posts: " << totalLikes << std::endl;
    if (!feed.empty())
        std::cout << "[+] Average likes per post: " << (totalLikes / static_cast<int>(feed.size())) << std::endl;
    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: comments - total comment counts across posts
// ============================================================================
static int cmd_comments(const std::vector<std::string>& args) {
    if (!checkReady("comments")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 20;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching comment counts for @" << target.username << " (last " << maxPosts << " posts)..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found (account may be private)" << std::endl;
        return 0;
    }

    int totalComments = 0;
    std::cout << std::endl;

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];
        std::string caption = item.caption.substr(0, 50);
        if (item.caption.length() > 50) caption += "...";
        if (caption.empty()) caption = "(no caption)";

        std::replace(caption.begin(), caption.end(), '\n', ' ');

        std::cout << "  Post " << (i + 1) << ": " << item.commentCount << " comments"
                  << " | " << item.mediaType
                  << " | " << caption << std::endl;
        totalComments += item.commentCount;
    }

    std::cout << std::endl;
    std::cout << "[+] Total comments across " << feed.size() << " posts: " << totalComments << std::endl;
    if (!feed.empty())
        std::cout << "[+] Average comments per post: " << (totalComments / static_cast<int>(feed.size())) << std::endl;
    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: hashtags - extract all hashtags used by target
// ============================================================================
static int cmd_hashtags(const std::vector<std::string>& args) {
    if (!checkReady("hashtags")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 30;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Extracting hashtags from @" << target.username << "'s posts..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    std::map<std::string, int> hashtagCount;
    for (const auto& item : feed) {
        for (const auto& tag : item.hashtags) {
            hashtagCount[tag]++;
        }
    }

    if (hashtagCount.empty()) {
        std::cout << "[*] No hashtags found in posts" << std::endl;
        return 0;
    }

    // Sort by frequency
    std::vector<std::pair<std::string, int>> sorted(hashtagCount.begin(), hashtagCount.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::cout << "[+] Found " << sorted.size() << " unique hashtag(s):" << std::endl << std::endl;

    for (const auto& [tag, count] : sorted) {
        std::cout << "  " << tag << " (used " << count << " time" << (count > 1 ? "s" : "") << ")" << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: captions - get captions from posts
// ============================================================================
static int cmd_captions(const std::vector<std::string>& args) {
    if (!checkReady("captions")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 10;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching captions from @" << target.username << "'s posts..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found" << std::endl;
        return 0;
    }

    std::cout << std::endl;

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];
        std::cout << "--- Post " << (i + 1) << " [" << item.mediaType << "] "
                  << formatTimestamp(item.takenAt) << " ---" << std::endl;

        if (item.caption.empty())
            std::cout << "(no caption)" << std::endl;
        else
            std::cout << item.caption << std::endl;

        std::cout << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: mediatype - count of photos vs videos
// ============================================================================
static int cmd_mediatype(const std::vector<std::string>& args) {
    if (!checkReady("mediatype")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 50;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Analyzing media types for @" << target.username << "..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    int photos = 0, videos = 0, carousels = 0;
    for (const auto& item : feed) {
        if (item.mediaType == "photo") photos++;
        else if (item.mediaType == "video") videos++;
        else if (item.mediaType == "carousel") carousels++;
    }

    std::cout << std::endl;
    std::cout << "[+] Media type breakdown (from " << feed.size() << " posts):" << std::endl;
    std::cout << "  Photos:    " << photos << std::endl;
    std::cout << "  Videos:    " << videos << std::endl;
    std::cout << "  Carousels: " << carousels << std::endl;
    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: photodesc - descriptions of post content
// ============================================================================
static int cmd_photodesc(const std::vector<std::string>& args) {
    if (!checkReady("photodesc")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 10;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching post descriptions for @" << target.username << "..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found" << std::endl;
        return 0;
    }

    std::cout << std::endl;

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];
        std::cout << "  Post " << (i + 1) << ":" << std::endl;
        std::cout << "    Type:     " << item.mediaType << std::endl;
        std::cout << "    Date:     " << formatTimestamp(item.takenAt) << std::endl;
        std::cout << "    Likes:    " << item.likeCount << std::endl;
        std::cout << "    Comments: " << item.commentCount << std::endl;
        std::cout << "    Location: " << (item.location.empty() ? "(none)" : item.location) << std::endl;

        std::string caption = item.caption;
        if (caption.length() > 100) caption = caption.substr(0, 100) + "...";
        std::replace(caption.begin(), caption.end(), '\n', ' ');
        std::cout << "    Caption:  " << (caption.empty() ? "(none)" : caption) << std::endl;

        if (!item.taggedUsers.empty()) {
            std::cout << "    Tagged:   ";
            for (size_t j = 0; j < item.taggedUsers.size(); j++) {
                if (j > 0) std::cout << ", ";
                std::cout << "@" << item.taggedUsers[j];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: addrs - get addresses from photos
// ============================================================================
static int cmd_addrs(const std::vector<std::string>& args) {
    if (!checkReady("addrs")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 50;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Extracting locations/addresses from @" << target.username << "'s posts..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    std::vector<std::pair<std::string, std::string>> locations;
    for (const auto& item : feed) {
        if (!item.location.empty()) {
            locations.emplace_back(item.location, item.locationAddress);
        }
    }

    if (locations.empty()) {
        std::cout << "[*] No geotagged locations found in posts" << std::endl;
        return 0;
    }

    std::cout << "[+] Found " << locations.size() << " geotagged location(s):" << std::endl << std::endl;

    for (size_t i = 0; i < locations.size(); i++) {
        std::cout << "  " << (i + 1) << ". " << locations[i].first;
        if (!locations[i].second.empty())
            std::cout << " - " << locations[i].second;
        std::cout << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: tagged - users tagged by the target
// ============================================================================
static int cmd_tagged(const std::vector<std::string>& args) {
    if (!checkReady("tagged")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << "[*] Finding users tagged by @" << target.username << "..." << std::endl;

    auto tagged = mgr.FetchTaggedUsers(target.userId);

    if (tagged.empty()) {
        std::cout << "[*] No tagged users found in posts" << std::endl;
        return 0;
    }

    std::cout << "[+] Found " << tagged.size() << " tagged user(s):" << std::endl << std::endl;

    for (size_t i = 0; i < tagged.size(); i++) {
        std::cout << "  " << (i + 1) << ". @" << tagged[i].username << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: u.comments - find where the target has commented + who comments
//                       on the target's posts
// ============================================================================
static int cmd_ucomments(const std::vector<std::string>& args) {
    if (!checkReady("u.comments")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 20;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    // --- Part 1: Who comments on the target's posts ---
    std::cout << "[*] Scanning @" << target.username << "'s posts for commenters..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    std::map<std::string, int> commenterCount;
    // Also track where the target commented on their own posts
    std::vector<std::pair<std::string, std::string>> targetSelfComments; // (post caption, comment text)

    for (const auto& item : feed) {
        if (item.commentCount == 0) continue;

        auto comments = mgr.FetchMediaComments(item.mediaId, 50);
        for (const auto& c : comments) {
            commenterCount[c.username]++;
            if (c.username == target.username) {
                std::string cap = item.caption.substr(0, 40);
                std::replace(cap.begin(), cap.end(), '\n', ' ');
                targetSelfComments.emplace_back(cap, c.text);
            }
        }
    }

    if (!commenterCount.empty()) {
        std::vector<std::pair<std::string, int>> sorted(commenterCount.begin(), commenterCount.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::cout << "[+] Found " << sorted.size() << " unique commenter(s) on target's posts:" << std::endl << std::endl;
        for (const auto& [username, count] : sorted) {
            std::cout << "  @" << username << " (" << count << " comment"
                      << (count > 1 ? "s" : "") << ")" << std::endl;
        }
        std::cout << std::endl;
    } else {
        std::cout << "[*] No commenters found on target's posts" << std::endl << std::endl;
    }

    // --- Part 2: Scan posts the target is tagged in for the target's comments ---
    std::cout << "[*] Scanning posts where @" << target.username << " is tagged for their comments..." << std::endl;

    auto taggedPosts = mgr.FetchUsertagsFeed(target.userId, maxPosts);

    std::vector<std::tuple<std::string, std::string, std::string>> targetTaggedComments; // (poster, comment text, date)
    std::map<std::string, int> taggedPostOwners; // who posts photos that tag the target

    for (const auto& item : taggedPosts) {
        // First element of taggedUsers is the post owner (set by FetchUsertagsFeed)
        std::string owner = item.taggedUsers.empty() ? "(unknown)" : item.taggedUsers[0];
        taggedPostOwners[owner]++;

        if (item.commentCount == 0) continue;

        auto comments = mgr.FetchMediaComments(item.mediaId, 50);
        for (const auto& c : comments) {
            if (c.username == target.username) {
                targetTaggedComments.emplace_back(owner, c.text, c.createdAt);
            }
        }
    }

    if (!taggedPosts.empty()) {
        std::cout << "[+] Found " << taggedPosts.size() << " post(s) where @" << target.username
                  << " is tagged" << std::endl << std::endl;
    }

    if (!targetTaggedComments.empty()) {
        std::cout << "[+] @" << target.username << " commented on " << targetTaggedComments.size()
                  << " tagged post(s):" << std::endl << std::endl;
        for (size_t i = 0; i < targetTaggedComments.size(); i++) {
            auto& [owner, text, date] = targetTaggedComments[i];
            std::cout << "  " << (i + 1) << ". On @" << owner << "'s post";
            if (!date.empty() && date != "0")
                std::cout << " (" << formatTimestamp(date) << ")";
            std::cout << ":" << std::endl;
            std::cout << "     \"" << text << "\"" << std::endl;
        }
        std::cout << std::endl;
    } else if (!taggedPosts.empty()) {
        std::cout << "[*] @" << target.username << " hasn't commented on any tagged posts" << std::endl << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: u.tagged - find posts where others tagged the target (usertags feed)
// ============================================================================
static int cmd_utagged(const std::vector<std::string>& args) {
    if (!checkReady("u.tagged")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 30;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Finding posts where @" << target.username << " is tagged..." << std::endl;

    auto taggedPosts = mgr.FetchUsertagsFeed(target.userId, maxPosts);

    if (taggedPosts.empty()) {
        std::cout << "[*] No tagged posts found (account may be private or no tags exist)" << std::endl;
        return 0;
    }

    // Collect unique users who tagged the target + post details
    std::map<std::string, std::vector<size_t>> userPosts; // username -> post indices

    for (size_t i = 0; i < taggedPosts.size(); i++) {
        // First taggedUser is the post owner (set by FetchUsertagsFeed)
        std::string owner = taggedPosts[i].taggedUsers.empty() ? "(unknown)" : taggedPosts[i].taggedUsers[0];
        userPosts[owner].push_back(i);
    }

    std::cout << "[+] Found " << taggedPosts.size() << " post(s) from " << userPosts.size()
              << " user(s) tagging @" << target.username << ":" << std::endl << std::endl;

    // Sort users by number of times they tagged the target
    std::vector<std::pair<std::string, std::vector<size_t>>> sorted(userPosts.begin(), userPosts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second.size() > b.second.size(); });

    for (const auto& [owner, indices] : sorted) {
        std::cout << "  @" << owner << " (" << indices.size() << " post"
                  << (indices.size() > 1 ? "s" : "") << "):" << std::endl;
        for (size_t idx : indices) {
            const auto& item = taggedPosts[idx];
            std::string caption = item.caption;
            if (caption.size() > 60) caption = caption.substr(0, 57) + "...";
            std::replace(caption.begin(), caption.end(), '\n', ' ');
            std::cout << "    - " << formatTimestamp(item.takenAt) << " | "
                      << item.mediaType << " | "
                      << (caption.empty() ? "(no caption)" : caption) << std::endl;
        }
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: photos - download target's photos
// ============================================================================
static int cmd_photos(const std::vector<std::string>& args) {
    if (!checkReady("photos")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPhotos = 5;
    if (!args.empty()) {
        try { maxPhotos = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching photo URLs from @" << target.username << " (max " << maxPhotos << ")..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPhotos);

    if (feed.empty()) {
        std::cout << "[*] No posts found" << std::endl;
        return 0;
    }

    std::cout << std::endl;
    std::cout << "[+] Photo URLs for @" << target.username << ":" << std::endl << std::endl;

    int photoNum = 0;
    for (const auto& item : feed) {
        if (!item.imageUrl.empty()) {
            photoNum++;
            std::cout << "  [" << photoNum << "] " << item.mediaType << " | "
                      << formatTimestamp(item.takenAt) << std::endl;
            std::cout << "      " << item.imageUrl << std::endl << std::endl;
        }
    }

    if (photoNum == 0) {
        std::cout << "[*] No photo URLs available" << std::endl;
    } else {
        std::cout << "[*] " << photoNum << " photo URL(s) listed" << std::endl;
        std::cout << "[*] Use curl or wget to download the images" << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: pfp - download profile picture
// ============================================================================
static int cmd_pfp(const std::vector<std::string>& args) {
    if (!checkReady("pfp")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << "[*] Fetching profile picture for @" << target.username << "..." << std::endl;

    auto hdUrl = mgr.FetchProfilePicHD(target.userId);

    std::cout << std::endl;
    if (hdUrl.has_value() && !hdUrl->empty()) {
        std::cout << "[+] HD Profile Picture URL:" << std::endl;
        std::cout << "    " << hdUrl.value() << std::endl;
    } else if (!target.profilePicUrl.empty()) {
        std::cout << "[+] Profile Picture URL (standard):" << std::endl;
        std::cout << "    " << target.profilePicUrl << std::endl;
    } else {
        std::cout << "[*] No profile picture URL available" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "[*] Use curl or wget to download the image" << std::endl;
    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: stories - download current stories
// ============================================================================
static int cmd_stories(const std::vector<std::string>& args) {
    if (!checkReady("stories")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << "[*] Fetching stories for @" << target.username << "..." << std::endl;

    auto stories = mgr.FetchStories(target.userId);

    if (stories.empty()) {
        std::cout << "[*] No active stories found for @" << target.username << std::endl;
        return 0;
    }

    std::cout << "[+] Found " << stories.size() << " active story/stories:" << std::endl << std::endl;

    for (size_t i = 0; i < stories.size(); i++) {
        const auto& s = stories[i];
        std::cout << "  Story " << (i + 1) << " [" << s.mediaType << "] "
                  << formatTimestamp(s.takenAt) << std::endl;

        if (!s.imageUrl.empty())
            std::cout << "    Image: " << s.imageUrl << std::endl;
        if (!s.videoUrl.empty())
            std::cout << "    Video: " << s.videoUrl << std::endl;
        std::cout << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: likers - see who liked specific posts
// ============================================================================
static int cmd_likers(const std::vector<std::string>& args) {
    if (!checkReady("likers")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 5;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching likers for @" << target.username << "'s posts..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found" << std::endl;
        return 0;
    }

    // Track most frequent likers across posts
    std::map<std::string, int> likerFrequency;

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];
        if (item.likeCount == 0) continue;

        std::cout << std::endl << "--- Post " << (i + 1) << " (" << item.likeCount << " likes) ---" << std::endl;

        auto likers = mgr.FetchMediaLikers(item.mediaId, 50);

        for (const auto& liker : likers) {
            std::cout << "  @" << liker.username;
            if (!liker.fullName.empty())
                std::cout << " (" << liker.fullName << ")";
            std::cout << std::endl;
            likerFrequency[liker.username]++;
        }
    }

    if (!likerFrequency.empty()) {
        // Show top likers (most frequent across posts)
        std::vector<std::pair<std::string, int>> sorted(likerFrequency.begin(), likerFrequency.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::cout << std::endl << "[+] Most frequent likers:" << std::endl;
        int shown = 0;
        for (const auto& [user, count] : sorted) {
            if (shown >= 20) break;
            std::cout << "  @" << user << " (liked " << count << " of " << feed.size() << " posts)" << std::endl;
            shown++;
        }
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: post.comments - see all comments on specific posts
// ============================================================================
static int cmd_postcomments(const std::vector<std::string>& args) {
    if (!checkReady("post.comments")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxPosts = 5;
    if (!args.empty()) {
        try { maxPosts = std::stoi(args[0]); } catch (...) {}
    }

    std::cout << "[*] Fetching comments for @" << target.username << "'s posts..." << std::endl;

    auto feed = mgr.FetchUserFeed(target.userId, maxPosts);

    if (feed.empty()) {
        std::cout << "[*] No posts found" << std::endl;
        return 0;
    }

    for (size_t i = 0; i < feed.size(); i++) {
        const auto& item = feed[i];

        std::string caption = item.caption.substr(0, 60);
        if (item.caption.length() > 60) caption += "...";
        std::replace(caption.begin(), caption.end(), '\n', ' ');

        std::cout << std::endl << "--- Post " << (i + 1) << ": " << caption << " ---" << std::endl;
        std::cout << "    (" << item.commentCount << " comments, " << item.likeCount << " likes)" << std::endl;

        if (item.commentCount == 0) {
            std::cout << "    (no comments)" << std::endl;
            continue;
        }

        auto comments = mgr.FetchMediaComments(item.mediaId, 30);

        for (const auto& c : comments) {
            std::cout << std::endl;
            std::cout << "  @" << c.username << " (" << formatTimestamp(c.createdAt) << "):" << std::endl;
            std::cout << "    " << c.text << std::endl;
            if (c.likeCount > 0)
                std::cout << "    [" << c.likeCount << " likes]" << std::endl;
        }
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: mutual - find mutual followers/following
// ============================================================================
static int cmd_mutual(const std::vector<std::string>& args) {
    if (!checkReady("mutual")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    int maxCount = 100;

    std::cout << "[*] Finding mutual connections for @" << target.username << "..." << std::endl;

    auto followers = mgr.FetchFollowers(target.userId, maxCount);
    auto following = mgr.FetchFollowing(target.userId, maxCount);

    // Find mutual (both following and followed by)
    std::set<std::string> followerSet;
    for (const auto& f : followers) {
        followerSet.insert(f.username);
    }

    std::vector<std::string> mutuals;
    for (const auto& f : following) {
        if (followerSet.count(f.username) > 0) {
            mutuals.push_back(f.username);
        }
    }

    std::cout << std::endl;
    std::cout << "[+] Followers: " << followers.size() << std::endl;
    std::cout << "[+] Following: " << following.size() << std::endl;
    std::cout << "[+] Mutual connections: " << mutuals.size() << std::endl;
    std::cout << std::endl;

    if (!mutuals.empty()) {
        std::cout << "[+] Mutual connections (follows each other):" << std::endl;
        for (size_t i = 0; i < mutuals.size(); i++) {
            std::cout << "  " << (i + 1) << ". @" << mutuals[i] << std::endl;
        }
        std::cout << std::endl;
    }

    // Show who target follows but doesn't follow them back
    std::set<std::string> followingSet;
    for (const auto& f : following) {
        followingSet.insert(f.username);
    }

    std::vector<std::string> notFollowingBack;
    for (const auto& f : followers) {
        if (followingSet.count(f.username) == 0) {
            notFollowingBack.push_back(f.username);
        }
    }

    if (!notFollowingBack.empty() && notFollowingBack.size() <= 30) {
        std::cout << "[+] Followers NOT followed back (" << notFollowingBack.size() << "):" << std::endl;
        for (const auto& u : notFollowingBack) {
            std::cout << "  @" << u << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: bio - show target biography
// ============================================================================
static int cmd_bio(const std::vector<std::string>& args) {
    if (!checkReady("bio")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << std::endl;
    std::cout << "[+] Bio for @" << target.username << ":" << std::endl;
    std::cout << std::endl;
    if (target.biography.empty())
        std::cout << "  (no biography set)" << std::endl;
    else
        std::cout << "  " << target.biography << std::endl;
    std::cout << std::endl;

    if (!target.externalUrl.empty()) {
        std::cout << "[+] External URL: " << target.externalUrl << std::endl;
        std::cout << std::endl;
    }

    return 0;
}

// ============================================================================
// Command: email - show target's public email
// ============================================================================
static int cmd_email(const std::vector<std::string>& args) {
    if (!checkReady("email")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << std::endl;
    if (!target.email.empty()) {
        std::cout << "[+] Public email for @" << target.username << ": " << target.email << std::endl;
    } else {
        std::cout << "[*] No public email found for @" << target.username << std::endl;
    }

    if (!target.phoneNumber.empty()) {
        std::cout << "[+] Public phone for @" << target.username << ": " << target.phoneNumber << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command: fwercount - show follower/following count
// ============================================================================
static int cmd_fwercount(const std::vector<std::string>& args) {
    if (!checkReady("fwercount")) return 1;

    auto& mgr = IG::SessionManager::Instance();
    auto target = mgr.GetTarget();

    std::cout << std::endl;
    std::cout << "[+] @" << target.username << " statistics:" << std::endl;
    std::cout << "  Posts:     " << target.mediaCount << std::endl;
    std::cout << "  Followers: " << target.followerCount << std::endl;
    std::cout << "  Following: " << target.followingCount << std::endl;

    if (target.followerCount > 0) {
        double ratio = static_cast<double>(target.followingCount) / target.followerCount;
        std::cout << "  Follow ratio (following/followers): " << std::fixed << std::setprecision(2) << ratio << std::endl;
    }

    std::cout << std::endl;
    return 0;
}

// ============================================================================
// Command dispatch map
// ============================================================================
using InstaFunc = int(*)(const std::vector<std::string>&);

static const std::map<std::string, InstaFunc> instaFuncMap = {
    {"info",          cmd_info},
    {"followers",     cmd_followers},
    {"followings",    cmd_followings},
    {"likes",         cmd_likes},
    {"comments",      cmd_comments},
    {"hashtags",      cmd_hashtags},
    {"captions",      cmd_captions},
    {"mediatype",     cmd_mediatype},
    {"photodesc",     cmd_photodesc},
    {"addrs",         cmd_addrs},
    {"tagged",        cmd_tagged},
    {"u.comments",    cmd_ucomments},
    {"u.tagged",      cmd_utagged},
    {"photos",        cmd_photos},
    {"pfp",           cmd_pfp},
    {"stories",       cmd_stories},
    {"likers",        cmd_likers},
    {"post.comments", cmd_postcomments},
    {"mutual",        cmd_mutual},
    {"bio",           cmd_bio},
    {"email",         cmd_email},
    {"fwercount",     cmd_fwercount},
};

// ============================================================================
// Main command entry point
// ============================================================================
int cmd_handle(const char* cmd, int argc, char** argv, int envc, char** env_map) {
    std::string command(cmd);

    auto it = instaFuncMap.find(command);
    if (it == instaFuncMap.end()) {
        std::cerr << command << ": unknown Instagram command" << std::endl;
        return 1;
    }

    // Build args from argv
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }

    return it->second(args);
}

int runlib_entry_point(int argc, char** argv) {
    return 0;
}
