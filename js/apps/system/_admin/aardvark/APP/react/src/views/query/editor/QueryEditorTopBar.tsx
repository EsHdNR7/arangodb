import { SingleSelect } from "@arangodb/ui";
import {
  Button,
  Flex,
  Icon,
  IconButton,
  Tooltip,
  useDisclosure
} from "@chakra-ui/react";
import React from "react";
import { MagicWand } from "styled-icons/boxicons-solid";
import { Bug } from "styled-icons/fa-solid";
import { ExternalLink } from "../../../components/link/ExternalLink";
import { useQueryContext } from "../QueryContextProvider";
import { DebugPackageModal } from "./DebugPackageModal";
import { QuerySpotlight } from "./QuerySpotlight";

export const QueryEditorTopBar = () => {
  const {
    setCurrentView,
    onQueryChange,
    currentQueryName,
    queryValue,
    queryBindParams,
    resetEditor,
    setResetEditor,
    onOpenSpotlight
  } = useQueryContext();
  const showNewButton =
    currentQueryName !== "" ||
    queryValue !== "" ||
    JSON.stringify(queryBindParams) !== "{}";
  const onNewQuery = () => {
    onQueryChange({
      value: "",
      parameter: {},
      name: ""
    });
    setResetEditor(!resetEditor);
  };
  const {
    isOpen: isDebugPackageModalOpen,
    onOpen: onOpenDebugPackageModal,
    onClose: onCloseDebugPackageModal
  } = useDisclosure();
  const version = window.versionHelper.getDocuVersion();
  return (
    <Flex
      gap="2"
      direction="row"
      paddingY="2"
      borderBottom="1px solid"
      borderBottomColor="gray.300"
    >
      <Button
        size="sm"
        colorScheme="gray"
        onClick={() => {
          setCurrentView("saved");
        }}
      >
        Saved Queries
      </Button>

      <DebugPackageModal
        isDebugPackageModalOpen={isDebugPackageModalOpen}
        onCloseDebugPackageModal={onCloseDebugPackageModal}
      />
      {showNewButton && (
        <Button size="sm" colorScheme="gray" onClick={onNewQuery}>
          New
        </Button>
      )}
      <Flex marginLeft="auto" gap="4" alignItems="center">
        <ExternalLink
          href={`https://docs.arangodb.com/${version}/develop/http-api/queries/aql-queries/#create-a-cursor`}
        >
          Docs
        </ExternalLink>
        <Tooltip label="Create debug package">
          <IconButton
            colorScheme="gray"
            onClick={onOpenDebugPackageModal}
            icon={<Icon as={Bug} />}
            aria-label={"Create debug package"}
          />
        </Tooltip>
        <IconButton
          onClick={onOpenSpotlight}
          icon={<Icon as={MagicWand} />}
          aria-label={"Spotlight"}
        />
        <QueryLimit />
      </Flex>
      <QuerySpotlight />
    </Flex>
  );
};

const QueryLimit = () => {
  const { setQueryLimit, queryLimit } = useQueryContext();
  const options = [
    { label: "100 results", value: "100" },
    { label: "250 results", value: "250" },
    { label: "500 results", value: "500" },
    { label: "1000 results", value: "1000" },
    { label: "2500 results", value: "2500" },
    { label: "5000 results", value: "5000" },
    { label: "10000 results", value: "10000" },
    { label: "All results", value: "all" }
  ];
  return (
    <SingleSelect
      value={options.find(option => {
        if (queryLimit === "all" && option.value === "all") {
          return option;
        }
        return Number(option.value) === Number(queryLimit);
      })}
      onChange={value => {
        if (!value) {
          return;
        }
        setQueryLimit(value.value === "all" ? "all" : Number(value.value));
      }}
      isClearable={false}
      options={options}
    />
  );
};
