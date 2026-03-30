from common.dumped_save import DumpedSaveReader, CatPedigree
from collections import deque

SAVE_DUMP_PATH_PREFIX = '../out/dumped_save'

'''
Sketch that recomputes coefficient of inbreeding and checks against oracle values from the save.

polymeric 2026
'''

# This COI calculation follows the algorithm implemented in the game
# Mewgenics.exe (SHA-256 25ae2f2fbd3c13faa04c69f5f1494330423ad1e268b41de73bb5cd9ac0590ac7)
# 14076eee0    double score_coefficient_of_inbreeding(struct Pedigree* arg1, int64_t parent_a_sql_key, int64_t parent_b_sql_key)
def calculate_coi_mewgenics(pedigree, memo, parent_a_id, parent_b_id):
    # The memo is keyed by cat ID pairs in numerically sorted order
    workstack = [tuple(sorted([parent_a_id, parent_b_id]))]

    while workstack:
        pair = workstack[-1]
        cat_x_id = pair[0]
        cat_y_id = pair[1]

        # Use previously memoized value if found
        if pair in memo:
            workstack.pop()
            continue

        # Game marks starter and stray cats' parents with key -1
        # Those cats have zero COI
        if cat_x_id == -1 or cat_y_id == -1:
            memo[pair] = 0.0
            workstack.pop()
            continue

        # If a parent was not found in the pedigree map (paranoid/corrupted save handling?),
        # treat the cat as a zero COI cat
        if cat_x_id not in pedigree or cat_y_id not in pedigree:
            memo[pair] = 0.0
            workstack.pop()
            continue

        # The rest of the function uses the concept of the coefficient of kinship, ϕ

        # ϕ and COI are tied by a key identity:
        # COI(child) = ϕ(child.parent_a, child.parent_b)

        # ϕ is recursively defined as follows:
        # ϕ(x, y) = 0.5(ϕ(x, y.parent_a) + ϕ(x, y.parent_b)) [recursion]
        # ϕ(y, y) = 0.5(1 + COI(y)) [base]

        # Because:
        # - all starter cats and strays have zero COI,
        # - all cats can trace their origins to a starter or stray cat,
        # calculations using the recursive definition of ϕ are assured to reach a base case.

        # ϕ(y, y) = 0.5(1 + COI(y))
        if cat_x_id == cat_y_id:
            # Another way to think of 'pedigree[cat_y_id].coi' is that it is a memoization of
            # "calculate_coi_mewgenics(pedigree, memo, pedigree[cat_y_id].parent_a_id, pedigree[cat_y_id].parent_b_id)"
            memo[pair] = 0.5 * (1.0 + pedigree[cat_y_id].coi)
            workstack.pop()
            continue

        pair_x_cross_y_parent_a = tuple(sorted([cat_x_id, pedigree[cat_y_id].parent_a_id]))
        pair_x_cross_y_parent_b = tuple(sorted([cat_x_id, pedigree[cat_y_id].parent_b_id]))

        # ϕ(x, y) = 0.5(ϕ(x, y.parent_a) + ϕ(x, y.parent_b))
        if pair_x_cross_y_parent_a in memo and pair_x_cross_y_parent_b in memo:
            # Fetch ϕ(x, y.parent_a) and ϕ(x, y.parent_b) from the memo
            memo[pair] = 0.5 * (memo[pair_x_cross_y_parent_a] + memo[pair_x_cross_y_parent_b])
            workstack.pop()
        else:
            # Recursively calculate ϕ(x, y.parent_a) or ϕ(x, y.parent_b) if not already in memo
            if pair_x_cross_y_parent_a not in memo:
                workstack.append(pair_x_cross_y_parent_a)
            if pair_x_cross_y_parent_b not in memo:
                workstack.append(pair_x_cross_y_parent_b)
            # Don't pop the current stack entry, as we need to circle back after the two dependencies have been memoized

    return memo[tuple(sorted([parent_a_id, parent_b_id]))]

# This COI calculation is shamefully inefficient (exponential due to enumerating all paths),
# but hopefully is a good reference, as it was sketched based off reading the Wikipedia
# "coefficient of inbreeding" article
def calculate_coi_naive(pedigree, memo, parent_a_id, parent_b_id):
    # COI_child := sum_COMMONANCESTORS=ca(0.5**(path_through_parent_a_to_ca+path_through_parent_b_to_ca-1) * (1 + COI_ca))
    # where for each COMMONANCESTORS=ca, path_through_parent_a_to_ca intersect path_through_parent_b_to_ca == {ca}
    if parent_a_id == -1 or parent_b_id == -1:
        return 0.0
    else:
        # Stupid implementation: find common ancestors by set-intersecting all fan-ins to parent_a and parent_b
        # Inbreeding loops are identified as pairs of paths that set-intersect with exactly one common member
        # and which both end at that common member. That common set member is called the "common ancestor".
        # There is a 1-1 correspondence between loops and the common ancestor at the top of each loop.
        def list_fanins(id):
            witnessed = set()
            # current_node, accumulated_path
            workstack = deque([(id,)])
            while workstack:
                path = workstack.pop()
                if path in witnessed:
                    continue
                else:
                    witnessed.add(path)
                    last_node = pedigree[path[-1]]
                    if last_node.parent_a_id >= 0:
                        path_plus = (*path, last_node.parent_a_id)
                        workstack.append(path_plus)
                    if last_node.parent_b_id >= 0:
                        path_plus = (*path, last_node.parent_b_id)
                        workstack.append(path_plus)
            return witnessed

        fanins_parent_a = list_fanins(parent_a_id)
        fanins_parent_b = list_fanins(parent_b_id)

        # Find all common ancestors and push their COI and the loop length to a list
        common_ancestors = []
        for fanin_a in fanins_parent_a:
            set_fanin_a = set(fanin_a)
            for fanin_b in fanins_parent_b:
                set_fanin_b = set(fanin_b)
                intersection = set_fanin_a.intersection(set_fanin_b)
                if len(intersection) == 1 and fanin_a[-1] == fanin_b[-1]:
                    ca_id = fanin_a[-1]
                    ca_coi = pedigree[ca_id].coi
                    # total loop members excluding child node
                    generational_exponent = len(fanin_a) + len(fanin_b) - 1
                    common_ancestors.append((ca_id, ca_coi, generational_exponent))

        # Now compute the kitten's COI
        coi = 0.0
        for ca_info in common_ancestors:
            ca_id, ca_coi, generational_exponent = ca_info
            coi += 0.5 ** generational_exponent * (1 + ca_coi)
        return coi

def compute_all_cois(pedigree):
    # Iterate through all cats in the pedigree map and
    # recalculate all COI numbers from scratch.

    # This function isn't based off any code from the game, but simply
    # repeats what was naturally calculated as strays were added/
    # kittens born in a save over time.

    # Topological sort to order ancestors before descendants
    s0_indegrees = {id: 0 for id in pedigree.keys()}
    for id in s0_indegrees.keys():
        node = pedigree[id]
        for parent_id in [node.parent_a_id, node.parent_b_id]:
            if parent_id >= 0:
                s0_indegrees[parent_id] += 1

    s0_workqueue = deque([k for k, v in s0_indegrees.items() if v == 0])
    s1_workqueue = deque([])
    while s0_workqueue:
        id = s0_workqueue.popleft()
        node = pedigree[id]
        s1_workqueue.appendleft(id)
        for parent_id in [node.parent_a_id, node.parent_b_id]:
            if parent_id >= 0:
                s0_indegrees[parent_id] -= 1
                if s0_indegrees[parent_id] == 0:
                    s0_workqueue.append(parent_id)

    # New pedigree map that contains COI values we computed
    # We rebuild it based off the game's parent information, one ancestor at a time
    our_pedigree = {}
    our_coi_memos = {}
    for id in s1_workqueue:
        # Only use parent information from the original pedigree entry (their_pedigree_entry.parent_a_id and their_pedigree_entry.parent_b_id)
        # Notably, we do not use their_pedigree_entry.coi. Doing so would defeat the purpose of this exercise.
        their_pedigree_entry = pedigree[id]
        # our_coi = calculate_coi_naive(our_pedigree, our_coi_memos, their_pedigree_entry.parent_a_id, their_pedigree_entry.parent_b_id)
        our_coi = calculate_coi_mewgenics(our_pedigree, our_coi_memos, their_pedigree_entry.parent_a_id, their_pedigree_entry.parent_b_id)
        our_pedigree[id] = CatPedigree(id, their_pedigree_entry.parent_a_id, their_pedigree_entry.parent_b_id, our_coi)

    return our_pedigree

def map_name(cats, id):
    if id in cats:
        return cats[id].name()
    elif id == -1:
        return "ROOT"
    else:
        return "UNKNOWN"

def map_coi(coi):
    if coi >= 0.25:
        return 'slightly'
    elif coi >= 0.50:
        return 'moderately'
    elif coi >= 0.80:
        return 'highly'
    else:
        return 'not'

def main():
    save_reader = DumpedSaveReader(SAVE_DUMP_PATH_PREFIX)
    cats = save_reader.read_cats()
    pedigree, coi_memos, accessible_cats = save_reader.read_pedigree()

    cois = compute_all_cois(pedigree)
    for k, v in pedigree.items():
        our_coi = cois[k].coi
        their_coi = v.coi
        # check the coi value stored in the pedigree structure vs. ours
        if our_coi != their_coi:
            print('FAIL', our_coi, their_coi)
            raise Exception

    for k, v in sorted(cats.items()):
        # check the coi field stored in the cat structure vs. ours
        our_pedigree_entry = cois[k]
        our_coi = our_pedigree_entry.coi
        their_coi = v.coi()
        if our_coi != their_coi:
            print('FAIL', our_coi, their_coi)
            raise Exception
        parent_a = map_name(cats, our_pedigree_entry.parent_a_id)
        parent_b = map_name(cats, our_pedigree_entry.parent_b_id)
        print(f'{v.name()} ({our_coi:.2f}, {map_coi(our_coi)} inbred) <- {parent_a} * {parent_b}')

if __name__ == '__main__':
    main()
